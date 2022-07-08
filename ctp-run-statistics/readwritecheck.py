#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Wed Jul  6 12:08:49 2022

@author: feng
"""

DataRead = {}#{'13:50:00.500':{"IC2209": timestamp, "IF2207":timestamp}}
TwoSourcesData = {}
OneSourceGaps = {}
TwoSourceGaps = {}
import csv
import numpy
import pandas
from scipy.stats import ttest_1samp
def processlines(rows, result_dict):
    for row in rows:
        if len(row)==5:
            arrivaltime, exchangetime, exchangemillisec, ticker,latency = row
        else:
            arrivaltime, exchangetime,  ticker,latency = row
            exchangemillisec='0'
        updatetime = '.'.join([exchangetime, exchangemillisec])
        result_dict.setdefault( updatetime, {} ).setdefault( ticker, (float(arrivaltime), float(latency)) )
    
    
        
    

def run():
    
    ''' '''
    readlines = list(csv.reader(open('./ctp_read_stats.csv')))[1:]
    twosourcelines = list(csv.reader(open('./ctp_accept_IC2209_stats.csv')))[1:]
    twosourcelines += list(csv.reader(open('./ctp_accept_IF2207_stats.csv')))[1:]
    processlines(readlines, DataRead)
    processlines(twosourcelines, TwoSourcesData)
    for updatetime, info in TwoSourcesData.items():
        if updatetime not in DataRead:
            print( "missing updatetime %s in data read"%updatetime )
        else:
            inforead = DataRead[updatetime]
            for ticker in info.keys():
                if ticker not in inforead:
                    print('missing:(%s, %s) in data read'%(updatetime, ticker))
                    
