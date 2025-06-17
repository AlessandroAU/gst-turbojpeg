#!/usr/bin/env python3
"""
Test runner for GST TurboJPEG benchmarks and pattern generation.

This script provides automated testing and benchmarking capabilities for the
TurboJPEG encoder/decoder performance with SMPTE color bar test patterns.
"""

import subprocess
import sys
import os
import argparse
import json
import time
from pathlib import Path
from typing import Dict, List, Optional, Tuple

class TestRunner:
    def __init__(self, build_dir: str = "builddir"):
        self.build_dir = Path(build_dir)
        self.project_root = Path(__file__).parent
        self.encoder_benchmark = self.build_dir / "benchmarks" / "encoder_benchmark"
        self.decoder_benchmark = self.build_dir / "benchmarks" / "decoder_benchmark"
        self.pattern_viewer = self.build_dir / "benchmarks" / "pattern_viewer"
        
    def check_binaries(self) -> bool:
        """Check if all required binaries exist and are executable."""
        binaries = [self.encoder_benchmark, self.decoder_benchmark, self.pattern_viewer]
        missing = []
        
        for binary in binaries:
            if not binary.exists():
                missing.append(str(binary))
            elif not os.access(binary, os.X_OK):
                missing.append(f"{binary} (not executable)")
        
        if missing:
            print(f"❌ Missing or non-executable binaries:")
            for binary in missing:
                print(f"   - {binary}")
            print(f"\n💡 Run 'meson compile -C {self.build_dir}' to build them.")
            return False
        
        print("✅ All required binaries found and executable")
        return True
    
    def run_command(self, cmd: List[str], timeout: int = 300) -> Tuple[bool, str, str]:
        """Run a command and return success status, stdout, and stderr."""
        try:
            print(f"🔄 Running: {' '.join(str(c) for c in cmd)}")
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=timeout,
                cwd=self.project_root
            )
            return result.returncode == 0, result.stdout, result.stderr
        except subprocess.TimeoutExpired:
            return False, "", f"Command timed out after {timeout} seconds"
        except Exception as e:
            return False, "", str(e)
    
    def build_project(self) -> bool:
        """Build the project using meson."""
        print("🔨 Building project...")
        
        # Check if build directory exists
        if not self.build_dir.exists():
            print(f"📁 Creating build directory: {self.build_dir}")
            success, stdout, stderr = self.run_command([
                "meson", "setup", str(self.build_dir)
            ])
            if not success:
                print(f"❌ Failed to setup build directory:")
                print(f"   stdout: {stdout}")
                print(f"   stderr: {stderr}")
                return False
        
        # Compile benchmarks
        success, stdout, stderr = self.run_command([
            "meson", "compile", "-C", str(self.build_dir),
            "benchmarks/encoder_benchmark",
            "benchmarks/decoder_benchmark", 
            "benchmarks/pattern_viewer"
        ])
        
        if success:
            print("✅ Build completed successfully")
            return True
        else:
            print(f"❌ Build failed:")
            print(f"   stdout: {stdout}")
            print(f"   stderr: {stderr}")
            return False
    
    def generate_test_patterns(self, output_dir: str = "test_patterns") -> bool:
        """Generate test patterns for visual inspection."""
        print(f"🎨 Generating test patterns in {output_dir}/...")
        
        # Create output directory
        pattern_dir = Path(output_dir)
        pattern_dir.mkdir(exist_ok=True)
        
        patterns = ["smpte_color_bars", "gradient", "checkerboard"]
        # patterns = ["smpte_color_bars", "mixed_frequency", "gradient", "checkerboard"]
        resolutions = [
            ("720p", 1280, 720),
            ("1080p", 1920, 1080),
            ("4k", 3840, 2160)
        ]
        
        all_success = True
        
        for pattern in patterns:
            for res_name, width, height in resolutions:
                output_prefix = pattern_dir / f"{pattern}_{res_name}"
                
                success, stdout, stderr = self.run_command([
                    str(self.pattern_viewer),
                    "-p", pattern,
                    "-w", str(width),
                    "-h", str(height),
                    "-o", str(output_prefix),
                    "-q", "85"
                ])
                
                if success:
                    print(f"   ✅ Generated {pattern} at {res_name}")
                else:
                    print(f"   ❌ Failed to generate {pattern} at {res_name}")
                    print(f"      stderr: {stderr}")
                    all_success = False
        
        if all_success:
            print(f"✅ All test patterns generated successfully in {output_dir}/")
        
        return all_success
    
    def run_encoder_benchmark(self, min_time: float = 0.5, repetitions: int = 1) -> Optional[Dict]:
        """Run encoder benchmark and parse results."""
        print(f"🚀 Running encoder benchmark (min_time={min_time}s, reps={repetitions})...")
        
        success, stdout, stderr = self.run_command([
            str(self.encoder_benchmark),
            f"--benchmark_min_time={min_time}",
            f"--benchmark_repetitions={repetitions}",
            "--benchmark_format=json"
        ], timeout=600)
        
        if not success:
            print(f"❌ Encoder benchmark failed:")
            print(f"   stderr: {stderr}")
            return None
        
        try:
            # Parse JSON output from benchmark
            # Google Benchmark outputs JSON on stdout
            result_data = json.loads(stdout)
            print("✅ Encoder benchmark completed successfully")
            return result_data
        except json.JSONDecodeError as e:
            print(f"❌ Failed to parse encoder benchmark JSON output: {e}")
            print(f"   Raw output: {stdout[:500]}...")
            return None
    
    def run_decoder_benchmark(self, min_time: float = 0.5, repetitions: int = 1) -> Optional[Dict]:
        """Run decoder benchmark and parse results."""
        print(f"🔍 Running decoder benchmark (min_time={min_time}s, reps={repetitions})...")
        
        success, stdout, stderr = self.run_command([
            str(self.decoder_benchmark),
            f"--benchmark_min_time={min_time}",
            f"--benchmark_repetitions={repetitions}",
            "--benchmark_format=json"
        ], timeout=600)
        
        if not success:
            print(f"❌ Decoder benchmark failed:")
            print(f"   stderr: {stderr}")
            return None
        
        try:
            result_data = json.loads(stdout)
            print("✅ Decoder benchmark completed successfully")
            return result_data
        except json.JSONDecodeError as e:
            print(f"❌ Failed to parse decoder benchmark JSON output: {e}")
            print(f"   Raw output: {stdout[:500]}...")
            return None
    
    def format_benchmark_results(self, encoder_data: Dict, decoder_data: Dict) -> str:
        """Format benchmark results for display."""
        report = []
        report.append("=" * 80)
        report.append("📊 TURBOJPEG BENCHMARK RESULTS")
        report.append("=" * 80)
        report.append("")
        
        # System info
        if "context" in encoder_data:
            context = encoder_data["context"]
            report.append(f"🖥️  System: {context.get('host_name', 'Unknown')}")
            report.append(f"💻 CPU: {context.get('num_cpus', 'Unknown')} cores")
            report.append(f"📅 Date: {context.get('date', 'Unknown')}")
            report.append("")
        
        # Encoder results
        report.append("🚀 ENCODER PERFORMANCE (SMPTE Color Bars)")
        report.append("-" * 50)
        
        if "benchmarks" in encoder_data:
            for benchmark in encoder_data["benchmarks"]:
                name = benchmark.get("name", "Unknown")
                
                # Skip statistical entries (mean, median, stddev, cv)
                if any(stat in name for stat in ["_mean", "_median", "_stddev", "_cv"]):
                    continue
                
                time_ms = benchmark.get("cpu_time", 0) / 1e6  # Convert ns to ms
                fps = benchmark.get("FPS", 0)
                throughput_mbps = benchmark.get("bytes_per_second", 0) / 1e6
                
                report.append(f"{name:45} {time_ms:8.2f}ms {fps:8.1f} FPS {throughput_mbps:8.1f} MB/s")
        
        report.append("")
        
        # Decoder results
        report.append("🔍 DECODER PERFORMANCE (SMPTE Color Bars)")
        report.append("-" * 50)
        
        if "benchmarks" in decoder_data:
            for benchmark in decoder_data["benchmarks"]:
                name = benchmark.get("name", "Unknown")
                
                # Skip statistical entries (mean, median, stddev, cv)
                if any(stat in name for stat in ["_mean", "_median", "_stddev", "_cv"]):
                    continue
                
                time_ms = benchmark.get("cpu_time", 0) / 1e6
                fps = benchmark.get("FPS", 0)
                throughput_mbps = benchmark.get("bytes_per_second", 0) / 1e6
                
                report.append(f"{name:45} {time_ms:8.2f}ms {fps:8.1f} FPS {throughput_mbps:8.1f} MB/s")
        
        report.append("")
        report.append("=" * 80)
        
        return "\n".join(report)
    
    def save_results(self, encoder_data: Dict, decoder_data: Dict, filename: str = "benchmark_results.json"):
        """Save benchmark results to a JSON file."""
        results = {
            "timestamp": time.time(),
            "date": time.strftime("%Y-%m-%d %H:%M:%S"),
            "encoder": encoder_data,
            "decoder": decoder_data
        }
        
        with open(filename, 'w') as f:
            json.dump(results, f, indent=2)
        
        print(f"💾 Results saved to {filename}")

def main():
    parser = argparse.ArgumentParser(description="Run TurboJPEG benchmarks and tests")
    parser.add_argument("--build-dir", default="builddir", 
                       help="Build directory (default: builddir)")
    parser.add_argument("--build", action="store_true",
                       help="Build project before running tests")
    parser.add_argument("--patterns", action="store_true",
                       help="Generate test patterns")
    parser.add_argument("--pattern-dir", default="test_patterns",
                       help="Directory for generated patterns (default: test_patterns)")
    parser.add_argument("--benchmark", action="store_true",
                       help="Run performance benchmarks")
    parser.add_argument("--min-time", type=float, default=0.5,
                       help="Minimum benchmark time in seconds (default: 0.5)")
    parser.add_argument("--repetitions", type=int, default=1,
                       help="Number of benchmark repetitions (default: 1)")
    parser.add_argument("--save-results", default="benchmark_results.json",
                       help="Save benchmark results to file (default: benchmark_results.json)")
    parser.add_argument("--all", action="store_true",
                       help="Run all tests: build, patterns, and benchmarks")
    
    args = parser.parse_args()
    
    if not any([args.build, args.patterns, args.benchmark, args.all]):
        parser.print_help()
        print("\n💡 Use --all to run everything, or specify individual options")
        return 1
    
    runner = TestRunner(args.build_dir)
    
    print("🧪 TurboJPEG Test Runner")
    print("=" * 40)
    
    # Build if requested or if running all
    if args.build or args.all:
        if not runner.build_project():
            return 1
    
    # Check binaries exist
    if not runner.check_binaries():
        return 1
    
    # Generate patterns if requested
    if args.patterns or args.all:
        if not runner.generate_test_patterns(args.pattern_dir):
            print("⚠️  Pattern generation had some failures")
    
    # Run benchmarks if requested
    if args.benchmark or args.all:
        encoder_results = runner.run_encoder_benchmark(args.min_time, args.repetitions)
        decoder_results = runner.run_decoder_benchmark(args.min_time, args.repetitions)
        
        if encoder_results and decoder_results:
            # Display formatted results
            report = runner.format_benchmark_results(encoder_results, decoder_results)
            print("\n" + report)
            
            # Save results
            runner.save_results(encoder_results, decoder_results, args.save_results)
        else:
            print("❌ Benchmark execution failed")
            return 1
    
    print("\n✅ All tests completed successfully!")
    return 0

if __name__ == "__main__":
    sys.exit(main())