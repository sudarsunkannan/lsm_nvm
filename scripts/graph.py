import matplotlib
matplotlib.use('Agg')  # Must be before importing matplotlib.pyplot or pylab!
import matplotlib.pyplot as plt
import matplotlib.cm as cmx
import matplotlib.colors as colors
import numpy as np
import math
import urllib2
import numpy as np
from matplotlib.ticker import MultipleLocator, FormatStrFormatter



def sensitivity_graph_gen(x_label, title, filename, num_benchmarks, benchmarks, xlistoflist, ylistoflist):
    legend = []
    fig1, ax = plt.subplots()
    ax.set_xscale('log')


    for i in range(num_benchmarks):
        yplot_list = []
        xplot_list = []
        for yitem, xitem in zip(ylistoflist, xlistoflist):
          if len(yitem) and len(xitem) > 0:
            print yitem
            #yplot_list.append(float(yitem[i]));
            #xplot_list.append(float(xitem[i]));
            print "*******"
            my_xticks = xitem
            plt.xticks(xitem, xitem)
            #plt.xlim(min(my_xticks), max(my_xticks))
            #plt.xticks(np.linspace(min(my_xticks), max(my_xticks), len(my_xticks), endpoint=True)) 
            line, = plt.plot(xitem, yitem)
            plt.setp(line, marker='o', linestyle="--")
            legend.append(line)
            i = i + 1
    plt.xlabel(x_label)
    plt.ylabel('Throughput')
    plt.title(title)
    plt.legend(legend, benchmarks, loc='best')
    plt.savefig(filename, bbox_inches='tight')


def graph_gen(x_label, title, filename, num_benchmarks, benchmarks, x_values, y_values):
    plt.xlabel(x_label)
    plt.ylabel('Throughput')
    plt.title(title)
    legend = []

    for i in range(num_benchmarks):
        plot_list = [item[i] for item in y_values]
        line, = plt.plot(x_values, plot_list)
        plt.setp(line, marker='o', linestyle="--")
        legend.append(line)

    plt.legend(legend, benchmarks, loc='best')
    plt.savefig(filename, bbox_inches='tight')


def bar_graph_gen(x_label, title, filename, num_benchmarks, benchmarks, x_values, y_values):

    legend = []
    num_tests = len(x_values)
    width = 0.6
    x_width = math.ceil((width * float(num_tests)) + 0.2)

    ind = np.arange(0, num_benchmarks * int(x_width), int(x_width))

    cmap = get_cmap(int(num_benchmarks))
    plot_rect = []

    fig, ax = plt.subplots()
    ax.set_ylabel('Throughput')
    ax.set_xticks(ind + (width * num_tests / 2))
    ax.set_xticklabels(benchmarks)
    ax.set_title(title)

    Std = [1 for i in range(num_benchmarks)]

    xplot_list = [int(z) for z in x_values]

    for i in range(num_tests):
        col = cmap(i)
        plot_list = y_values[i]
        plot_list = [float(j) for j in plot_list]
        rects = ax.bar(ind + (width * i), plot_list, width, color=col, yerr=Std)
        plot_rect.append(rects)
        # print y_values[i]
        # print y_values

    legend_label = []
    for label in x_values:
        legend_label.append(x_label + str(label))

    ax.legend(plot_rect, legend_label, loc='best')

    for rects in plot_rect:
        autolabel(rects, ax)

    plt.savefig(filename, bbox_inches='tight')


def autolabel(rects, ax):
    # attach some text labels
    for rect in rects:
        height = rect.get_height()
        ax.text(rect.get_x() + rect.get_width() / 2., 1.05 * height,
                '%d' % int(height),
                ha='center', va='bottom')


def get_cmap(N):
    '''Returns a function that maps each index in 0, 1, ... N-1 to a distinct
    RGB color.'''
    color_norm = colors.Normalize(vmin=0, vmax=N - 1)
    scalar_map = cmx.ScalarMappable(norm=color_norm, cmap='hsv')

    def map_index_to_rgb_color(index):
        return scalar_map.to_rgba(index)
    return map_index_to_rgb_color
