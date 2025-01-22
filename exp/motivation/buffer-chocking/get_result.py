import os
import pandas as pd
import sys
# 将 mylib 所在的目录添加到 sys.path
parent_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(parent_dir)
from general import helper
method_array = ["DT_b", "DT"]
x_array = ["1", "2", "3", "4", "5", "6", "7", "8", "9","10", "11", "12", "13", "14"]
x_implication = "request_size(/buffer size)"
normalized_base = "DT"

file_dirs = ["result/", "../no-bak/result/"]
figure_dir = "figure/"
query_common_figs = ['query_rct_ave',]
plt_params=[ (query_common_figs, (7, 6), None, "query", figure_dir, 0),]

def deal_folder(file_dirs, method_array, x_array):
    batch = 0
    flt_data = []
    for file_dir in file_dirs:
        for files in os.listdir(file_dir):
            path = file_dir + files
            method, x_val, ans = helper.deal_one_folder(path, method_array, x_array)
            if not bool(ans):
                continue
            for metric, value in ans.items():
                flt_data.append({
                    "Method": method, 
                    "X_val": float(x_val),
                    "Metric": metric,
                    "Value": float(value)
                })
            batch += 1
            progress = (batch / (len(method_array) * len(x_array))) * 100
    global df
    df = pd.DataFrame(flt_data)
    return df

def draw(df, plt_params, method_array, x_array, x_implication):
    batch = 0
    for plt_param in plt_params:
        helper.plot_with_params(df, plt_param, method_array, x_array, x_implication)
        batch += 1
        progress = (batch / (len(plt_params))) * 100
        print(f"\rPlotting file progress: {progress:.2f}%", end="")

def main():
    global data
    df = deal_folder(file_dirs, method_array, x_array)
    # df = helper.unitize_data(df, normalized_base)
    draw(df, plt_params, method_array, x_array, x_implication)    

if __name__ == '__main__':

    main()
