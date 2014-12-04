import matplotlib.pylab as plt
import binner_pb2
import sys
import datetime
from matplotlib.ticker import FuncFormatter

def timestamp_to_string(x, pos):
    return datetime.datetime.fromtimestamp(x / 1000.0 / 1000.0).strftime('%M:%S.%f')

binned_flows = binner_pb2.BinnedFlows()

f = open(sys.argv[1], "rb")
binned_flows.ParseFromString(f.read())
f.close()

for bin_pack in binned_flows.bin_packs:
    plt.figure()
    
    start = bin_pack.bins_start
    x = [k * bin_pack.bin_width for k in range(bin_pack.num_bins)]

    #print '\tBP type', bin_pack.type
    #print '\tBP start', bin_pack.bins_start
    #print '\tBP bin width', bin_pack.bin_width
    #print '\tBP num bins', bin_pack.num_bins
    total_max = 0

    for binned_values in bin_pack.values:
        max_value = max(binned_values.bins)
        if max_value > total_max:
            total_max = max_value
        
    for binned_values in bin_pack.values:
        max_value = max(binned_values.bins)
        if max_value < float(total_max) * 0.02:
            continue

        label = binner_pb2.FlowType.Name(binned_values.type)
        plt.plot(x, binned_values.bins, label=label)
        plt.axes().xaxis.set_major_formatter(FuncFormatter(timestamp_to_string))
        #print '\t\tValues', binned_values.bins

    plt.legend()

    label = binner_pb2.BinPack.Type.Name(bin_pack.type)
    bin_width = str(bin_pack.bin_width / 1000000.0)
    plt.title(label + ' grouped by time, bin width ' + bin_width + 'sec')

plt.show()