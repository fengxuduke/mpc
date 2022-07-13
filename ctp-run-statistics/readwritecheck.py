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
        elif len(row)==4:
            arrivaltime, exchangetime,  ticker,latency = row
            exchangemillisec='0'
        elif len(row)==3:
            arrivaltime, ticker,latency = row
            exchangemillisec='0'
            exchangetime=''
        updatetime = '.'.join([exchangetime, exchangemillisec])
        result_dict.setdefault( updatetime, {} ).setdefault( ticker, (float(arrivaltime), float(latency)) )
    
    
        
    

def run():
    
    ''' '''
    readlines = list(csv.reader(open('./ctp_read_stats_20220708.csv')))[1:]
    twosourcelines = list(csv.reader(open('./ctp_accept_IC2209_stats_20220708.csv')))[1:]
    twosourcelines += list(csv.reader(open('./ctp_accept_IF2207_stats_20220708.csv')))[1:]
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

def extractordersofpattern( orders, pattern='IC' ):
    extractedorders = set()
    for x in orders:
        extractedorder = tuple( contract for contract in x if pattern in contract )
        extractedorders.add(extractedorder)
    return extractedorders

def alllexisorted(  orders ):
    return all( [tuple(sorted(x))==tuple(x) for x in orders] )

def runtickerorders(feedpaths=['./ctp_accept_IC2209_stats_20220708.csv','./ctp_accept_IF2207_stats_20220708.csv' ],
                    readpaths=['./ctp_read_stats_20220708.csv']):
    readlines = []
    sourcelines = []
    for readpath in readpaths:
        readlines += list(csv.reader(open(readpath)))[1:]
    for feedpath in feedpaths:
        sourcelines += list(csv.reader(open(feedpath)))[1:]
    processlines(readlines, DataRead)
    processlines(sourcelines, TwoSourcesData)
    feedorders = set()
    readorders = set()
    for updatetime, info in TwoSourcesData.items():
        if updatetime<='09:31:00':
            continue
        currentfeedorder = tuple( sorted( tuple(info.keys()), key=lambda x: info[x][0] ) )
        feedorders.add(currentfeedorder)
        if updatetime not in DataRead:
            print( "missing updatetime %s in data read"%updatetime )            
        else:
            inforead = DataRead[updatetime]
            for ticker in info.keys():
                if ticker not in inforead:
                    print('missing:(%s, %s) in data read'%(updatetime, ticker))
            currentreadorder = tuple( sorted( tuple(inforead.keys()), key=lambda x: inforead[x][0] ) )
            readorders.add(currentreadorder)
            if currentfeedorder!=currentreadorder:
                print( "%s:unmatched feed and read order:%s,%s"%( updatetime, str(currentfeedorder), str(currentreadorder) ) )
    IFfeedorders = extractordersofpattern(feedorders, 'IF')
    ICfeedorders = extractordersofpattern(feedorders, 'IC')
    IHfeedorders = extractordersofpattern(feedorders, 'IH')
    feedorders2207 = extractordersofpattern(feedorders, '2207')
    feedorders2208 = extractordersofpattern(feedorders, '2208')
    feedorders2209 = extractordersofpattern(feedorders, '2209')
    feedorders2212 = extractordersofpattern(feedorders, '2212')
    
    for name, orders in zip( ['feedorders', 'IFfeedorders', 'ICfeedorders', 'IHfeedorders', 'feedorders2207', 'feedorders2208', 'feedorders2209', 'feedorders2212'],
                            [feedorders, IFfeedorders, ICfeedorders, IHfeedorders, feedorders2207, feedorders2208, feedorders2209, feedorders2212] ):
        print( 'is %s lexi-sorted?'%name, alllexisorted(orders) )
    
    return feedorders, readorders, IFfeedorders, ICfeedorders, IHfeedorders, feedorders2207, feedorders2208, feedorders2209, feedorders2212
            