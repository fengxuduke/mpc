/*上期所：
cu,bc,al,zn,pb,ni,sn,au,ag,rb,wr,hc,ss,sc,lu,fu,bu,ru,nr,sp
郑商所：(PF209)
SR,CF,PM,WH,RI,LR,JR,RM,RS,OI,CY,AP,CJ,PK,ZC,TA,MA,FG,SF,SM,UR,SA,PF
大商所：
c,cs,a,b,m,y,p,fb,bb,jd,rr,lh,l,v,pp,j,jm,i,eg,eb,pg
中金所：
IC,IF,IH,TS,TF,T
*/

#if !defined(EXCHANGEASSETS_H)
#define EXCHANGEASSETS_H

#include<vector>
#include<map>
#include<string>
using namespace std;

static std::map<std::string, std::vector<string>> ExchangeAssets = {  { "SHFE", {"cu","bc","al","zn","pb","ni","sn","au","ag","rb","wr","hc","ss","sc","lu","fu","bu","ru","nr","sp"} },
                                                                      { "ZCE",  {"SR","CF","PM","WH","RI","LR","JR","RM","RS","OI","CY","AP","CJ","PK","ZC","TA","MA","FG","SF","SM","UR","SA","PF"}},
                                                                      { "DCE",  {"c","cs","a","b","m","y","p","fb","bb","jd","rr","lh","l","v","pp","j","jm","i","eg","eb","pg" }},
                                                                      { "CFFE", {"IC","IF","IH","TS","TF","T"}}                                                        
                                                         
                                                                         };


#endif