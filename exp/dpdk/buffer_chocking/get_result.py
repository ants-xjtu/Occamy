import os
import pandas as pd
import sys
# 将 mylib 所在的目录添加到 sys.path
parent_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(parent_dir)
from general import helper
method_array = ["dt_b", "dt", "pBuffer_b", "pushout", "abm_b",
        "pushout_b", "pBuffer"]
method_array = ["Pushout0_b", "Pushout0", "DT0_b", "DT0", "ABM0_b", "ABM0", "Occamy0", "Occamy0_b"]
method_array = ["DT0_b", "Occamy0_b", "ABM0_b", "Pushout0_b"]
# query_size_list=(1.5 1.7 1.9 2.1 2.3 2.5)
x_array = ["1.5", "1.7", "1.9", "2.1",  "2.3", "2.5"]# "2.6", "2.7", "2.8", "2.9", "3.0", ]

x_implication = "request_size(/buffer size)"

file_dir = "result/"
figure_dir = "figure/"
query_common_figs = ['query_rct_ave', 'query_rct_99th', ]
plt_params=[ (query_common_figs, (14, 6), (1, 2), "query", figure_dir, 0),]
            

def deal_folder(file_dir, method_array, x_array):
    batch = 0
    flt_data = []
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
        print(f"\rReading file progress: {progress:.2f}%", end="")
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
    df = deal_folder(file_dir, method_array, x_array)
    draw(df, plt_params, method_array, x_array, x_implication)    

if __name__ == '__main__':
    main()
