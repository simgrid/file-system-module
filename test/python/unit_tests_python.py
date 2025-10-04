import subprocess
import sys

# List of script files to run
scripts = [
    "caching_test.py",
    "file_system_test.py",
    "jbod_storage_test.py",
    "one_disk_storage_test.py",
    "one_remote_disk_storage_test.py",
    "path_util_test.py",
    "register_test.py",
    "seek_test.py",
    "stat_test.py",
    "truncate_test.py"
    ]
                        
def run_script(script):
    print(f"\nRunning {script}...")
    try:
        result = subprocess.run([sys.executable, script], check=True, capture_output=True, text=True)
        print(f"✅ {script} completed successfully.")
        print(result.stdout)
        return True
    except subprocess.CalledProcessError as e:
        print(f"❌ {script} failed with error:")
        print(e.stderr)
        return False
                                                                                
if __name__ == "__main__":
    all_passed = True
    for script in scripts:
        if not run_script(script):
            all_passed = False
    if not all_passed:
        sys.exit(1)
