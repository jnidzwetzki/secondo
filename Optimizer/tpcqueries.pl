%
% November 2004, M. Spiekermann
%
% Some TPC-H queries in Secondo SQL syntax notated
% as prolog facts.

tpcQuery(1, select
	[ 
          count(*) as count_order,
          lreturnflag,
          llinestatus,
          sum(lquantity) as sum_qty,
          sum(lextendedprice) as sum_base_price,
          sum(lextendedprice * (1 - ldiscount)) as sum_disc_price,
          sum(lextendedprice * (1 - ldiscount) * (1 + ltax)) as sum_charge,
          avg(lquantity) as avg_qty,
	  avg(lextendedprice) as avg_price,
	  avg(ldiscount) as avg_disc
        ]
from
	  lineitem 
where
lshipdate < theInstant(1998,9,2)
groupby [
          lreturnflag,
          llinestatus
        ] 
orderby
	[ 
    lreturnflag asc,
    llinestatus asc 
  ]
).

tpcQuery(simple1, select
	[ 
      count(*) as count_order,
      lreturnflag,
      llinestatus,
      sum(lquantity) as sum_qty,
	    avg(ldiscount) as avg_disc
  ]
from
	  lineitem 
where
    lshipdate < theInstant(1998,9,2)
groupby [
          lreturnflag,
          llinestatus
        ] 
).

tpcQuery(10, select
      [
        ccustkey,
        cname,
        sum(lextendedprice * (1 - ldiscount)) as revenue,
        cacctbal,
        nname,
        caddress,
        cphone,
        ccomment
      ]
from
      [
        customer,
        orders,
        lineitem,
        nation
      ]
where
      [
        ccustkey = ocustkey,
        lorderkey = oorderkey,
        not(oorderdate < theInstant(1993,10,1)),
        oorderdate < theInstant(1994,1,1),
        lreturnflag = "R",
        cnationkey = nnationkey
      ]
groupby
      [
        ccustkey,
        cname,
        cacctbal,
        cphone,
        nname,
        caddress,
        ccomment
      ]
orderby [ revenue desc]
first 20 
).

tpcQuery(5, select
       [
        nname,
        sum(lextendedprice * (1 - ldiscount)) as revenue
       ]
from
       [
        customer,
        orders,
        lineitem,
        supplier,
        nation,
        region
       ]
where
       [
        ccustkey = ocustkey,
        lorderkey = oorderkey,
        lsuppkey = ssuppkey,
        cnationkey = snationkey,
        snationkey = nnationkey,
        nregionkey = rregionkey,
        rname = "ASIA", 
	((oorderdate >= theInstant(1994,1,1)) and (oorderdate < theInstant(1995,1,1)))
       ]
groupby [ nname ]
orderby [ revenue desc ]
).


tpcQuery(3, select
  [ 
    lorderkey,
    sum(lextendedprice * (1 - ldiscount)) as revenue,
    oorderdate,
    oshippriority 
  ]
from
	[ 
    customer,
    orders,
    lineitem
  ]
where
	[
    cmktsegment = "BUILDING", 
    ccustkey = ocustkey,
    lorderkey = oorderkey 
  ]
groupby
  [ 
    lorderkey,
    oorderdate,
    oshippriority 
  ]
orderby
  [ 
    revenue desc,
    oorderdate asc 
  ]
first 10
).


tpcQueryExt(3.1, select
  [ 
    oorderdate,
    oshippriority 
  ]
from
	[ 
    customer,
    orders
  ]
where
  [
    cmktsegment = "BUILDING", 
    ccustkey = ocustkey
  ]
first 10
).

tpcQueryExt(3.2, select
  [ 
    oorderdate,
    oshippriority 
  ]
from
  [ 
    orders,
    lineitem
  ]
where
  [
    lorderkey = oorderkey 
  ]
first 10
).




tpcQueryExt(3.3, select
  [ 
    oorderdate,
    oshippriority 
  ]
from
	[ 
    customer,
    orders,
    lineitem
  ]
where
  [
    cmktsegment = "BUILDING", 
    ccustkey = ocustkey,
    lorderkey = oorderkey 
  ]
first 10
).




% a variant of TPC-3 which includes some correlated predicates
tpcQueryExt(correl1, select
	[ 
          cnationkey,
          count(*) as sumX
        ]
        from
	[ 
          customer,
	  orders,
	  lineitem 
        ]
         where
	[
    lreceiptdate < (lshipdate + create_duration(30.0)),
    lcommitdate < (lshipdate + create_duration(30.0)),
    lcommitdate < (lreceiptdate + create_duration(30.0)),
    lreceiptdate > (theInstant(1996,1,1) + create_duration(30.0)),
    lcommitdate > (theInstant(1996,1,1) + create_duration(30.0)),
    lshipdate >  (theInstant(1996,1,1) + create_duration(30.0)),
    lquantity > 25, 
    ccustkey = ocustkey,
    lorderkey = oorderkey 
  ]
groupby [
          cnationkey 
        ] 

).

% a query which demonstrates the need of randomization
tpcQueryExt(random1,
  
  select [ l1:ldiscount, l1:lorderkey, 
           l1:lsuppkey, l1:lcomment, 
           o:oorderkey 
	 ] 
  from   [ lineitem as l1, orders as o, 
           lineitem2 as l2 
	 ] 
  where  [ l1:lorderkey = l2:lorderkey, 
           l1:lorderkey = o:oorderkey, 
           l2:lorderkey > 500 
	 ]
).



%% simple binary joins

tpcJP( bj1, part, ppartkey, partsupp, pspartkey).

tpcJP( bj2, supplier, ssuppkey, partsupp, pssuppkey).

tpcJP( bj3, supplier, snationkey, customer, cnationkey).
tpcJP( bj3a, supplier, sphone, customer, cphone).

tpcJP( bj4, partsupp, pspartkey, lineitem, lpartkey).
tpcJP( bj4a, partsupp, pssuppkey, lineitem, lsuppkey).

tpcJP( bj5, lineitem, lorderkey, orders, oorderkey).
tpcJP( bj5a, lineitem, lcommitdate, orders, oorderdate).

tpcJP( bj6, customer, ccustkey, orders, ocustkey).

tpcIndex( i1, lineitem, lorderkey ).
tpcIndex( i2, orders, oorderkey ).
tpcIndex( i3, partsupp, pssuppkey ).

% a rule for creating equi join queries from the predicates
% encoded in tpcJP.
tpcQuery2( X, select count(*) from [ R1, R2 ] where A1 = A2 ) :-
  tpcJP( X, R1, A1, R2, A2 ).
  
tpcBigScan('query LINEITEM_512MB feed count').

% create derived data if necessary
objectDef(lineitem_512MB, 'let LINEITEM_512MB = LINEITEM feed {A} LINEITEM feed {B} head[1500] product head[600000] consume').

objectDef(lineitem2, 'let LINEITEM2 = LINEITEM feed consume').

createIfNecessary(O) :-
  objectDef(O, Cmd),
  not(relation(O, _)),
  nl, write('Object '), write(O) , write(' needs to be created.'), nl,	
  runQuery(Cmd).

createIfNecessary(O) :-
  relation(O, _),	
  nl, write('Object '), write(O) , write(' is already present.'), nl.	

createObjects :-
  findall([O], createIfNecessary(O), _).



tpcGetQuery(No, X) :-
  ( not(tpcQuery(No, X)), not(tpcQuery2(No, X)) ) 
    -> write('There is no tpc query with label '), write(No), nl, fail 
     ; ( tpcQuery(No, X) ; tpcQuery2(No, X)).

%tpcGetQuery(No, X) :-
% tpcQuery(No, X).


sql2Query(Term, Query) :-
  defaultExceptionHandler((
  isDatabaseOpen,
  getTime( mOptimize(Term, Query, _), PlanBuild ),
  nl, write('Optimization time: '), write(PlanBuild), nl
  %%appendToRel('SqlHistory', Term, Query, Cost, PlanBuild, PlanExec)
 )).


tpc(No) :- tpcGetQuery(No, X), !, sql(X), !.

tpc(No) :- tpcQuery2(No, X), sql(X).


tpcOptimize(No) :- tpcGetQuery(No, X), !, optimize(X).
tpcAfterLookup(No) :- tpcGetQuery(No, X), callLookup(X,Y), !, write(Y).


tpcOptimize2(No) :- tpcQuery2(No, X), optimize(X).
tpcPlan2 :-
  findall( No, tpcOptimize2(No), L ), nl, write(L), nl.

tpcRun2 :-
  findall( No, tpcOptimize2(No), L ), nl, write(L), nl.



tpcCorrel(N) :-
  tpcCorrelated(N, T), sql(T). 

%%% 
%%% Experiments
%%%


attr_to_atom( Rel, Attr, Atom) :-
  spelled(Rel:Attr, attr(Name, Arg, Case)),
  plan_to_atom(a(Name, Arg, Case), Atom).

rel_to_atom2( Rel, Atom) :-
  lookupRel(Rel, Rel2),
  rel_to_atom(Rel2, Atom).  

sampleRel(RelAtom, S) :-
  concat_atom([RelAtom, '_sample_j'], S).


joinTerm(smj, R1, R2, A1, A2, Term) :-
  concat_atom([ R1,  ' feed ', R2, ' feed sortmergejoin[', A1, ', ',  A2, ']'], Term).

joinTerm(smjr2, R1, R2, A1, A2, Term) :-
  concat_atom([ R1,  ' feed ', R2, 
                ' feed sortmergejoin_r2[', A1, ', ',  A2, '] shuffle3 head[500]'], Term).

joinTerm(hash, R1, R2, A1, A2, Term) :-
  concat_atom([ R1,  ' feed ', R2, 
                ' feed hashjoin[', A1, ', ',  A2, ', 9997', '] shuffle3 head[500]'], Term).


joinQueryLet( Type, Ident, R1, R2, A1, A2, Q ) :-
  joinTerm(Type, R1, R2, A1, A2, T),	
  concat_atom(['let ', Ident, ' = ',  T, 'consume'], Q).


joinQueryCount( Type, R1, R2, A1, A2, Q ) :-
  joinTerm(Type, R1, R2, A1, A2, T),
  concat_atom(['query ', T, ' count'], Q).

% concat a term to a query which computes a equi width histogram over
% a given attribute of type integer or real; Parameters:
%
% Attr    : attribute name
% Width   : the width of a bucket
% Ntuples : number of tuples

histaggr(Attr, Width, Ntuples, Result) :-
  concat_atom( 
    [' sortby[ ', Attr, ' asc ] extend[Bucket: .', Attr, ' div ', Width, ' ]',
     ' groupby[Bucket; Percent: . count / ', Ntuples ,'] consume'], Result ).


% the dynamic predicate ~flag~ a tool for setting and querying options.

:- dynamic flag/2,
   clearflags.

clearflags :-
  retractall( flag(_,_) ).

setflag(F) :- assert( flag(F, on) ).
clearflag(F) :- retractall( flag(F, _) ).

showflag( [F, Val], _ ) :-
  nl, write(F), write(' --> '), write(Val), nl.	


showflags :-
  findall( [X, Y], flag(X, Y), L),
  maplist( showflag, L, _).  
  	


showQuery(Q) :-
  nl, write(Q), nl.

runQuery(Q) :-
  flag(runMode, on),
  nl, write('Executing '), write(Q), write(' ...'), nl, 	
  secondo(Q), !.

runQuery(Q) :-
  showQuery(Q).


runQuery(Q, Res) :-
  flag(runMode, on),
  nl, write('Executing '), write(Q), write(' ...'), nl, 	
  secondo(Q, [_, Res]), 
  nl, write('Result: '), write(Res), nl, !. 

runQuery(Q, Res) :-
  showQuery(Q),
  Res = 999,
  nl, write('Dummy-Result: '), write(Res), nl. 



letIndex(Rel, Attr, Q) :-
  rel_to_atom2(Rel, Rel_A),
  attr_to_atom(Rel, Attr, Attr_A),
  concat_atom([ 'let ', Rel_A, '_', Attr_A, ' = ', 
                Rel_A, ' createbtree[', Attr_A, ']' ], Q).

createIndex(X) :-
  tpcIndex(X, Rel, Attr), 
  letIndex(Rel, Attr, Q),
  runQuery(Q).

createIndexes :-
  findall(X, createIndex(X), _).

findop(O) :-
  concat_atom([ 'query SEC2OPERATORINFO feed filter[.Name contains "', 
                O, '"] consume' ], Q),
  runQuery(Q).  

% to do: small index names !!!
indexScan(Value, Q) :- 
  concat_atom(['query lineitem_lORDERKEY LINEITEM leftrange[', Value, ']'], Q). 


getSuffix(shuffle, ' shuffle3 count').
getSuffix(no_shuffle, ' head[100000] count').

indexQuery(Value, Q, S) :-
  indexScan(Value, Q_tmp),
  getSuffix(S, Q_tmp2),  
  concat_atom([Q_tmp, Q_tmp2], Q).


% rules for running a query which have a cache clearing effect.

clearCache :-
  tpcBigScan(Q),
  runQuery(Q).
         	
clearCache(clear_cache) :- clearCache. 

clearCache(keep_cache).



% Run some index scans

indexScanRun(Opt, Suffix, [H | T]) :-
  clearCache(Opt),
  indexQuery(H, Q, Suffix),
  runQuery(Q),
  indexScanRun(Opt, Suffix, T).
  
indexScanRun(_, _, []).

compareIndexScans(L) :-
  indexScanRun(keep_cache, shuffle, L),
  clearCache,  
  indexScanRun(keep_cache, no_shuffle, L),
  cmdHist2File('indexscan_keep_cache.csv'), 
  clearCache,  
  indexScanRun(clear_cache, shuffle, L),	
  indexScanRun(clear_cache, no_shuffle, L),	
  cmdHist2File('indexscan_clear_cache.csv'). 



% dump the command history to a given file name

cmdHist2File(Name) :-
  concat_atom(['query SEC2COMMANDS feed '], Q),
  dumpQueryResult2File(Q, Name, Q2),
  runQuery(Q2).
    
% dump the result of a secondo query to a CSV file

dumpQueryResult2File(Q, File, Q2) :-
  concat_atom([Q, ' dumpstream["', File, '","|"] tconsume'], Q2).



% create a relation wich contains 3 histograms

joinHists(X, Q) :-
  concat_atom([X, '_hist_complete'], R1),
  concat_atom([X, '_hist_smjr3'], R2),
  concat_atom([X, '_hist_sample'], R3),
  concat_atom(['query ', R1, ' feed {R1} ', 
                         R2, ' feed {R2} ', 	
                         R3, ' feed {R3} ',
		         ' sortmergejoin[Bucket_R2, Bucket_R3] ',	 
		         ' sortmergejoin[Bucket_R1, Bucket_R2] '], Q).


% create extended histogram data for query Q. Since spreadsheets programs
% like OpenOffice-Calc do not have a diagram type for histograms additional
% points are computed in order to use x-y plots instead.   

extendHistData(Q, Res) :-
  concat_atom([Q, 'extendstream[Dup: intstream(1,2)] ',
                  'addcounter[Cntx, 0] ',
		  'extend[No: ceil(.Cntx / 2)] '], Res). 


createOutputs(X) :-
  joinHists(X, Q),
  %concat_atom([X, '_hists.csv'], File),
  %dumpQueryResult2File(Q, File, Q2),  
  %runQuery(Q2),
  extendHistData(Q, Q2ext),
  concat_atom([Q2ext, 'project[No, Percent_R1, Percent_R2, Percent_R3]'], Q3),
  concat_atom([X, '_hists.csv'], File2),
  dumpQueryResult2File(Q3, File2, Q4),  
  runQuery(Q4).



histquery(Jointype, R1, R2, A1, A2, File):-
  joinQueryCount(Jointype, R1, R2, A1, A2, Q2),
  runQuery(Q2, Res), % compute the total number of result tuples
  histaggr(A1, 1000, Res, H), % add the groupby term
  joinQueryLet(Jointype, File, R1, R2, A1, A2, Q1),
  concat_atom([Q1, ' ', H], Q3),
  runQuery(Q3).


tpcRelAttr(X, R1a, R2a, A1a, A2a) :-
  tpcJP(X, R1, A1, R2, A2),
  rel_to_atom2(R1, R1a),
  rel_to_atom2(R2, R2a),
  attr_to_atom(R1, A1, A1a),
  attr_to_atom(R2, A2, A2a).


% create 3 histograms for the same join query
%
% 1: histogram of the complete join result
% 2: histogram of the join of two samples 
% 3: histogram of the random prefix hash-sort-merge, 
% 4: hashjoin, 
% 5: index-loop (if applicable)

histcompare(X) :-
  tpcRelAttr(X, R1a, R2a, A1a, A2a),	
  % compute histogram for the complete join result
  concat_atom([X, '_hist_complete'], F1),
  histquery(smj, R1a, R2a, A1a, A2a, F1),
  % compute histograms for the result using the sample relations
  sampleRel(R1a, R1as),
  sampleRel(R2a, R2as),
  concat_atom([X, '_hist_sample'], F2),
  histquery(smj, R1as, R2as, A1a, A2a, F2),
  % compute histogram for smj_r3
  concat_atom([X, '_hist_smjr2'], F3),
  histquery(smjr2, R1a, R2a, A1a, A2a, F3),
  % compute histogram for hashjoin
  concat_atom([X, '_hist_hj'], F3),
  histquery(hash, R1a, R2a, A1a, A2a, F3),
  createOutputs(X).

% run queries which do a performance comparision of sort-merge and
% hash-sort-merge.

perfcompare(X) :-
  tpcRelAttr(X, R1a, R2a, A1a, A2a),	
  joinQueryCount(smj, R1a, R2a, A1a, A2a, Q1),
  nl, write(Q1), nl, runQuery(Q1), runQuery(Q1),
  joinQueryCount(smjr2, R1a, R2a, A1a, A2a, Q2),
  nl, write(Q2), nl, runQuery(Q2), runQuery(Q2).


% user level commands for running experiments

experiment(compare_smj_performance, 'Run queries for performance comparisons between sortmerge and sortmerge_r2').

experiment(compare_attribute_distributions, 'Run queries which demonstrate the quality of randomness. The results are histograms over the join attribute').

experiment(compare_index_scans, 'Run queries which demonstrate the overhead for shuffling after index scan').

experiment(compare_tpc, 'Run TPC-H queries with and without adaptive join').

experiment(compare_tpc_idx, 'Run TPC-H queries with and without adaptive join, use of indexes is allowed').






showExperiment(X, Y) :-
  experiment(X, Y),
  nl, write(X), write(':'), 
  nl, write('----------------------------------'), 
  nl, write(Y), nl.  

showExperiments :-
  findall([X, Y], showExperiment(X,Y), _).	

showQueries(L) :-
  nl, write('Evaluated queries: '), write(L), nl.

compare_smj_performance :-
  findall(X, perfcompare(X), L),
  showQueries(L), 
  cmdHist2File('smj_perf.csv').

compare_attribute_distributions :-
  findall(X, histcompare(X), L),
  showQueries(L). 

compare_index_scans :-
  L = [1000, 2000, 3000, 4000, 5000],	
  compareIndexScans(L),
  showQueries(L). 


runtpc(Id) :-	
  tpcQuery(Id, Q) ; tpcQueryExt(Id, Q),
  sql2Query(Q, Cmd),
  runQuery(Cmd).

runtpc(_).


compare_tpc_queries(Param) :-	
  Q = [3, 5, 10, correl1, random1],
  delOption(adaptiveJoin),
  setOption(useCounters),
  setOption(noSymmjoin),
  (Param = useIndex -> delOption(noIndex) 
                      ; setOption(noIndex) ),
  checklist(runtpc, Q), !,
  setOption(adaptiveJoin),  
  checklist(runtpc, Q). 

compare_tpc :-
  compare_tpc_queries(noIndex).

compare_tpc_idx :-
  compare_tpc_queries(useIndex).


runAll :-
  findall(X, showExperiment(X,_), L),
  checklist(call, L).   
  

