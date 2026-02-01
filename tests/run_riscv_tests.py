#!/usr/bin/env python3
import os
import subprocess
from pathlib import Path
from colorama import Fore, Style

# Path configuration
EMU = "./emu"
TEST_DIR = "riscv-tests/isa"
OUTPUT_DIR = "output_logs"

# Which test prefixes to include (RV32IMAC)
INCLUDE_PREFIXES = ["rv32ui", "rv32mi", "rv32um", "rv32ua", "rv32uc","rv32si"]

# Which prefixes to skip (floating-point)

EXCLUDE_PREFIXES = ["rv32uf", "rv32ud"]

# Ensure output directory exists
os.makedirs(OUTPUT_DIR, exist_ok=True)

def run_test(test_path: Path):
    """Run a single ELF test and save its output."""
    test_name = test_path.stem
    output_file = Path(OUTPUT_DIR) / f"{test_name}.txt"
    print(f"🔹 Running {str(test_path)}")
    
    try:
        result = subprocess.run(
            [EMU,"--max_time=2000000","--mode=riscv-tests", "--elf="+str(test_path)],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=20,
            text=True
        )

        output_file.write_text(result.stdout)

        # Check for PASS marker
        if "PASS" in result.stdout or "pass" in result.stdout:
            status = f"{Fore.GREEN}PASS{Style.RESET_ALL}"
        else:
            status = f"{Fore.RED}FAIL{Style.RESET_ALL}"

    except subprocess.TimeoutExpired:
        status = f"{Fore.YELLOW}TIMEOUT{Style.RESET_ALL}"
    except Exception as e:
        status = f"{Fore.MAGENTA}ERROR ({e}){Style.RESET_ALL}"

    print(f"    → {status}")
    return test_name, status

def main():
    tests_by_group = {
        "rv32ui": [],
        "rv32mi": [],
        "rv32um": [],
        "rv32ua": [],
        "rv32uc": [],
        "rv32si": []
    }
    # Collect ELF test files
    for test_file in sorted(Path(TEST_DIR).glob("rv32*")):
        name = test_file.stem
        # Filter out excluded tests
        if any(name.startswith(pref) for pref in EXCLUDE_PREFIXES) or "dump" in str(test_file) or not os.path.isfile(str(test_file)):
            continue

        for prefix in INCLUDE_PREFIXES:
            if name.startswith(prefix):
                tests_by_group[prefix].append(test_file)
                break

    # Run tests
    results = []
    for group, files in tests_by_group.items():
        if not files:
            continue
        print(f"\n===  Running {group.upper()} tests ({len(files)} total) ===")
        for f in files:
            results.append((group, *run_test(f)))

    # Summary
    print("\n" + "="*60)
    print("TEST SUMMARY")
    print("="*60)

    for group in INCLUDE_PREFIXES:
        group_results = [r for r in results if r[0] == group]
        if not group_results:
            continue
        passed = sum("PASS" in r[2] for r in group_results)
        total = len(group_results)
        print(f"{group.upper():<10}: {passed}/{total} passed")

    print(f"\nAll output logs are saved in: {OUTPUT_DIR}/")

if __name__ == "__main__":
    main()
