import csv
import argparse

def parse_args():
    parser = argparse.ArgumentParser("Script to compare keyword results between 2 csv file")
    parser.add_argument("input1", type=str, help="Test input csv file")
    parser.add_argument("input2", type=str, help="Ref input csv file")
    parser.add_argument("--pass-threshold", type=int, default=1, help="Allowed keywords diff per test case. Default set to 1 keyword")
    return parser.parse_args()

def read_csv(input_file):
    with open(input_file, 'r') as csvfile:
        reader = csv.reader(csvfile)
        data = list(reader)
    return data


def compare_kwd(input1, input2, pass_threshold, verbose=True):
    data = [read_csv(input1), read_csv(input2)]
    kwd_dict = []
    
    index = 0
    for i in range(len(data)):
        kd = {}
        for j in range(1, len(data[i])):
            kd[data[i][j][0]] = int(data[i][j][3]) # Checking only Amazon WWE results
        kwd_dict.append(kd)
    
    not_found_in_file2 = []
    not_found_in_file1 = []
    diff = {}
    fail = {}
    for k1 in kwd_dict[0]: # Iterate over the first csv
        if k1 in kwd_dict[1]:
            d = kwd_dict[0][k1] - kwd_dict[1][k1]
            diff[k1] = d # If found in the second csv, log diff
            if abs(d) > pass_threshold:
                fail[k1] = d            
        else:
            not_found_in_file2.append(k1) # Otherwise keep track of everything not found

    for k1 in kwd_dict[1]: # Iterate over the second csv
        if k1 not in kwd_dict[0]: # If not found in first csv
            not_found_in_file1.append(k1) # keep track of everything not found
    
    if verbose:
        if len (diff):
            print("\nInput1 vs Input2 keywords diff")
            [print(key,': ',val) for key, val in diff.items()]
    
    if len(fail):
        print(f"\nERROR: FAILING test cases with error above pass threshold of {pass_threshold}:")
        [print(key,': ',val) for key, val in fail.items()]
        assert(False)
        
    if len(not_found_in_file2):
        print("\nERROR: Streams in file1 not found in file2")
        print(*not_found_in_file2, sep='\n')
        assert(False)

    if len(not_found_in_file1):
        print("\nERROR: Streams in file2 not found in file1")
        print(*not_found_in_file1, sep='\n')
        assert(False)
        
 
if __name__ == "__main__":
    args = parse_args();
    compare_kwd(args.input1, args.input2, args.pass_threshold)
    
