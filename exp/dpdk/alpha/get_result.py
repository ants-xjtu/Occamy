import os
import pandas as pd
import sys
# 将 mylib 所在的目录添加到 sys.path
parent_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(parent_dir)
from general import helper
method_array0 = ["Occamy3", "Occamy0", "Occamy1", "Occamy2", "Occamyf1"]
method_array1 = ["DT0", "DT1", "DT2", "DT3", "DTf1"]
x_array = ["0.3", "0.5", "0.7", "0.9", "1.0", "1.1", "1.2", "1.3", "1.4", "1.5", "1.6", "1.7", "1.8", "1.9"]
x_implication = "request_size(/buffer size)"
file_dir = "result/"
figure_dir = "figure/"
query_common_figs = ['query_rct_99th', ]
plt_params0=[ (query_common_figs, (7, 6), None, "query_dt", figure_dir, 0),]
plt_params1=[ (query_common_figs, (7, 6), None, "query_occamy", figure_dir, 0),]
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
    df = deal_folder(file_dir, method_array0, x_array)
    draw(df, plt_params0, method_array0, x_array, x_implication)    

    df = deal_folder(file_dir, method_array1, x_array)
    draw(df, plt_params1, method_array1, x_array, x_implication)

if __name__ == '__main__':

    main()
