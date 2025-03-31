def analyze_test_status(file_path):
    # Read the file
    with open(file_path, 'r') as file:
        lines = file.readlines()
    
    # Skip the header (first two lines)
    data_lines = lines[2:]
    
    total_tests = 0
    added_tests = 0
    not_added_tests = 0
    
    for line in data_lines:
        # Skip empty lines
        if not line.strip():
            continue
        
        # Parse the line
        parts = line.split('|')
        if len(parts) >= 3:  # Ensure we have enough columns
            total_tests += 1
            status = parts[2].strip()
            if status == 'YES':
                added_tests += 1
            elif status == 'NO' or status == '':
                not_added_tests += 1
    
    return total_tests, added_tests, not_added_tests

def main():
    file_path = 'status.txt'
    try:
        total, added, not_added = analyze_test_status(file_path)
        
        print(f"Test Status Analysis:")
        print(f"Total tests: {total}")
        print(f"Added tests: {added} ({(added/total)*100:.1f}%)")
        print(f"Not yet added tests: {not_added} ({(not_added/total)*100:.1f}%)")
        
    except FileNotFoundError:
        print(f"Error: The file '{file_path}' was not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    main()

