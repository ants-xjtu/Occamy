import os
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np

def average_fct_result(result):
    if(len(result) == 0):
        return 0
    return result['completion_time'].mean()

def cdf_fct_result(result, cdf):
    if(len(result) == 0):
        return 0
    return result['completion_time'].quantile(cdf)

def append_list(the_list, fileName):
    with open(fileName, "r") as f:
        for line in f.readlines():
            line = line.strip('\n').split()
            # size, completion time, dscp, sending rate, goodput
            the_list.append([int(line[0]), int(line[1]), int(line[2])])
    f.close()


def get_color(method):
    if(method[0:4] == "Push"):
        return "orangered"
    if(method[0] == 'O'):
        return "brown"
    if(method[0] == 'A'):
        return "green"
    return 'royalblue'

def get_markstyle(method):
    if(method[0:4] == "Push"):
        return "+"
    if(method[0] == 'O'):
        return "o"
    if(method[0] == 'A'):
        return "x"
    return '>'

def get_linestyle(method):
    alpha = method[-1]
    if(alpha == '0'):
        return '-'
    if(alpha == '1'):
        return '--'
    if(alpha == '2'):
        return '-.'
    if(alpha == '3'):
        return ':'
    if(alpha == '7'):
        return '--'
    if(method[-2:] == '_b'):
        return '--'
    if(alpha == '4' or  alpha == '5'):
        return ':'
    return '-'


def deal_one_folder(path, method_array, x_array):
    files = []
    for root, dirs, files in os.walk(path, topdown=False):
        files = files
    background_results = []
    query_results = []
    ans = {}
    if len(path.split('/')) < 2:
        return -1, -1, ans
    if(path.split('/')[1].split('-')[0] not in method_array):
        return -1, -1, ans
    method = path.split('/')[1].split('-')[0]
    x_val = np.nan
    if(len(path.split('/')[1].split('-')) >= 2):
        x_val = path.split('/')[1].split('-')[1]
        if(x_val not in x_array):
            return -1, -1, ans
    for file_name in files:
        file_path = os.path.join(path, file_name)
        if file_name.split("-")[1] == 'flows_incast.txt_reqs.txt':
            query_results.append(pd.read_csv(file_path, sep=" ", header=None, usecols=[0, 1, 2]))
        elif file_name.split("-")[1] == 'flows.txt':
            background_results.append(pd.read_csv(file_path, sep=" ", header=None, usecols=[0, 1, 2]))

    query_results = pd.concat(query_results)
    small = []
    medium = []
    large = []
    if len(background_results) != 0:
        background_results = pd.concat(background_results)
        background_results.columns = ['size', 'completion_time', 'dscp']
        small = background_results[background_results['size'] < 100 * 1024]
        medium = background_results[(background_results['size'] >= 100 * 1024) & (background_results['size'] < 10 * 1024 * 1024)]
        large = background_results[background_results['size'] >= 10 * 1024 * 1024]


    if not query_results.empty:
        query_results.columns = ['size', 'completion_time', 'dscp']
    
    ans = {
        "background_small_ave": average_fct_result(small),
        "background_small_99th": cdf_fct_result(small, 0.99),
        "background_large_ave": average_fct_result(large),
        "background_large_99th": cdf_fct_result(large, 0.99),
        "background_medium_ave": average_fct_result(medium),
        "background_medium_99th": cdf_fct_result(medium, 0.99),
        'query_rct_ave': average_fct_result(query_results),
        'query_rct_99th': cdf_fct_result(query_results, 0.99),
        'web_fct_ave': average_fct_result(background_results),
        'web_fct_99th': cdf_fct_result(background_results, 0.99),
    }
    return method, x_val, ans
def plot_with_params(df, plt_param, method_array, x_array, x_implication):
    figs, fig_size, lay, name2save, folder_name, is_normalized = plt_param
    plt.figure(figsize=fig_size)

    # 用于存储图例信息的列表
    lines = []
    labels = []

    for i in range(len(figs)):
        if lay:
            plt.subplot(lay[0], lay[1], i + 1)
        fig_name = figs[i]
        for method in method_array:
            if df is None:
                continue
            method_data = df[(df['Method'] == method) & (df['Metric'] == fig_name)]
            method_data_sorted = method_data.sort_values(by='X_val')


            line, = plt.plot(method_data_sorted["X_val"], method_data_sorted["Value"], color=get_color(method),
                                     linestyle=get_linestyle(method),
                                     marker=get_markstyle(method))
            plt.grid(True)
            if(method[-2] == 'f'):
                method_strlist = list(method)
                method_strlist[-2] = '-'
                method = ''.join(method_strlist)
             
            if method not in labels:
                lines.append(line)
                labels.append(method)
            
            plt.title(fig_name)
            plt.xlabel(x_implication)
            plt.ylabel(fig_name)

    plt.figlegend(lines, labels, loc='lower center')

    plt.tight_layout()
    plt.savefig(folder_name + name2save + ".png", bbox_inches='tight')
    plt.cla()

