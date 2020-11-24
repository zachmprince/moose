#!/usr/bin/env python3
#* This file is part of the MOOSE framework
#* https://www.mooseframework.org
#*
#* All rights reserved, see COPYRIGHT for full restrictions
#* https://github.com/idaholab/moose/blob/master/COPYRIGHT
#*
#* Licensed under LGPL 2.1, please see LICENSE for details
#* https://www.gnu.org/licenses/lgpl-2.1.html
import os
import argparse
import json
import collections
import mooseutils
import plotly
import plotly.graph_objects as go

def parse_args():
    parser = argparse.ArgumentParser(description='Tool for visualizing performance data.')
    parser.add_argument('file', type=argparse.FileType('r'))
    parser.add_argument('--name', required=True, help='Name of the PerfGraphReporter that outputted the data.')
    parser.add_argument('--visual', default='plot', choices=['plot', 'table'], help='Desired method of visualizing the data.')
    parser.add_argument('--type', default='tree', choices=['tree', 'basic', 'heaviest_branch', 'heaviest_sections'], help='What data to present.')

    parser.add_argument('--level', default=None, type=int, help='The level of detail to output. Higher levels will yield more detail. Used if --type=tree.')
    parser.add_argument('--num_sections', default=10, type=int, help='Number of sections to show when --type=heaviest_sections.')
    return parser.parse_args()

class PerfGraphData(object):
    """Class for converting a JSON PerfGraph to dictionary"""

    def __init__(self, file, name, type, **kwargs):
        kwargs.setdefault('num_sections', 10)

        self._data = dict()

        tmp = json_load(file)
        tstep = -1
        data = None
        for val in tmp['time_steps']:
            if val['time_step'] > tstep:
                tstep = tmp['time_step']
                for rep in val['reporters']:
                    if rep['object_name'] == name:
                        data = rep
                        break

        if data is None:
            raise mooseutils.MooseException('Could not find PerfGraphReporter.')

        if (type == 'tree'):
            self.initTree(data)
        elif (type == 'basic'):
            self.initBasic(data)
        elif (type == 'heaviest_branch'):
            self.initBranch(data)
        elif (type == 'heaviest_sections'):
            self.initSections(data, kwargs.get('num_sections'))
        else:
            raise mooseutils.MooseException('Unsupported PerfGraph type \'' + data['type'] + '\'')

       data['type'] = type

        @property
        def data(self):
            return self._data

        def initTree(self, data):
            self._data = data

        def initBasic(self, data):

        def initBranch(self, data):

        def initSections(self, data):



class PerfGraphPlot(object):
    """Class for outputting performance data to plot."""

    def __init__(self, data):
        self._total_runtime = data['total_runtime']
        self._data = []
        self._layout = dict()

        if (data['type'] == 'tree'):
            self.initTree(data)
        elif (data['type'] == 'basic'):
            self.initBasic(data)
        elif (data['type'] == 'heaviest_branch'):
            self.initBranch(data)
        elif (data['type'] == 'heaviest_sections'):
            self.initSections(data)
        else:
            raise mooseutils.MooseException('Unsupported PerfGraph type \'' + data['type'] + '\'')

    @property
    def data(self):
        return self._data

    @property
    def layout(self):
        return self._layout

    def initTree(self, data):
        xdata = []
        ydata = []
        names = []
        self.addBar(data['value'], xdata, ydata, names)

        num_entries = len(names)
        col = plotly.colors.DEFAULT_PLOTLY_COLORS
        y = [[0 for i in range(len(ydata))] for j in range(num_entries)]
        colors = [col[0] for j in range(num_entries)]
        k = 0
        for i in range(len(ydata)):
            for j in range(len(y[i])):
                y[k][i] = y[i][j]
                if j < len(col):
                    colors[k] = col[j]
                else:
                    colors[k] = col[k % len(col)]
                k = k + 1

        self._data = [dict() for i in range(num_entries)]
        for j in range(num_entries):
            self._data[j]['type'] = 'bar'
            self._data[j]['name'] = names[j]
            self._data[j]['x'] = xdata
            self._data[j]['y'] = y[j]
            self._data[j]['marker'] = dict(color=colors[j])

        self._layout['barmode'] = 'stack'
        self._layout['show_legend'] = False

    def initBasic(self, data):

    def initBranch(self, data):

    def initSections(self, data):

    def addBar(self, data, xdata, ydata, names):
        if data['num_children'] == 0:
            return

        xdata.append(data['name'])
        curry = [data['self']]
        names.append(data['name'] + ' (self)')

        for key, child in data:
            if 'child_' in key:
                curry.append(child['self'])
                names.append(child['name'])

        ydata.append(curry)

        for key, child in data:
            if 'child_' in key:
                self.addBar(child, xdata, ydata, names)

if __name__ == '__main__':
    args = parse_args()
    pg_data = PerfGraphData(*args.file, *args.name, *args.type)
    if *args.visual == 'plot':
        plot_data = PerfGraphPlot(pg_data.data())
        fig = go.Figure(data=plot_data.data(), layout=plot_data.layout())
        fig.show()
