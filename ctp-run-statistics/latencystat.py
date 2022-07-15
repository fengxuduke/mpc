#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Wed Jul  6 12:08:49 2022

@author: feng
"""

OneSourceData = {}#{'13:50:00.500':{"IC2209": timestamp, "IF2207":timestamp}}
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
    
    
        
    

def run( onesourcepaths = [ './ctp_accept_stats.csv' ], twosourcepaths = ['./ctp_accept_IF2208_stats.csv', './ctp_accept_IF2207_stats.csv' ] ):
    
    ''' '''
    onesourcelines=[]
    twosourcelines = []
    for onesourcepath in onesourcepaths:
        onesourcelines += list(csv.reader(open(onesourcepath)))[1:]
    for twosourcepath in twosourcepaths:        
        twosourcelines += list(csv.reader(open(twosourcepath)))[1:]
        print(twosourcepath, len(twosourcelines), twosourcelines[-1])
    processlines(onesourcelines, OneSourceData)
    processlines(twosourcelines, TwoSourcesData)
    startupdatetime = min( min(OneSourceData.keys()), min(TwoSourcesData.keys()) )
    for updatetime, info in TwoSourcesData.items():
        if updatetime<startupdatetime:
            continue
        if len(info.keys())<2:
            # print('<2 ticks for %s, skipped'%updatetime)
            continue
        ticktimestamps = sorted( list(info.items()), key=lambda x: x[1][0]  )
        TwoSourceGaps[updatetime] = ticktimestamps[1][1][0]-ticktimestamps[0][1][0]-ticktimestamps[1][1][1]
        if updatetime not in OneSourceData:
            # print('error: missing %s in onesourcedata'%updatetime)
            continue
        onesourceinfo = OneSourceData[updatetime]
        if ticktimestamps[0][0] not in onesourceinfo:
            # print('error: missing tick %s, %s'%(updatetime, ticktimestamps[0][0]) )
            OneSourceGaps[updatetime] = numpy.NaN
            continue
        if ticktimestamps[1][0] not in onesourceinfo:
            # print('error: missing tick %s, %s'%(updatetime, ticktimestamps[1][0]) )
            OneSourceGaps[updatetime] = numpy.NaN
            continue
        OneSourceGaps[updatetime] = onesourceinfo[ticktimestamps[1][0]][0] -onesourceinfo[ticktimestamps[0][0]][0]
    
    OneSourceGapsTS = pandas.Series(OneSourceGaps)
    TwoSourceGapsTS = pandas.Series(TwoSourceGaps)
    TwoMinusOneTS = TwoSourceGapsTS-OneSourceGapsTS
    
    
    #to student t-test for 0 mean
    #first remove outliers
    for name, ts in zip( ['OneSourceGaps', "TwoSourceGaps", "TwoSourceGaps-OneSourceGaps"], [OneSourceGapsTS, TwoSourceGapsTS, TwoMinusOneTS] ):
        condition = ( (ts-ts.mean()).abs()<=5*ts.std() ) & (~pandas.isna(ts))
        ts = ts[condition]
        t,p= ttest_1samp(ts,0.0)
        print("%s: mean=%s, the probability that mean=0 is <=%.4f"%(name, ts.mean(), p))
    return ts.mean(), ts.std(), t, p