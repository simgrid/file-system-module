import subprocess
import sys

# List of script files to run
scripts = [
    "caching_test.py",
    "file_system_test.py",
    "job_storage_test.py",
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
    except subprocess.CalledProcessError as e:
        print(f"❌ {script} failed with error:")
        print(e.stderr)
                                                                                
if __name__ == "__main__":
    for script in scripts:
        run_script(script)
                                                                                                            