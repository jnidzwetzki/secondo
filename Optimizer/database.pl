/*
1 Database Dependent Information

[File ~database.pl~]

1.1 Rules about Commutativity of Predicates

*/

commute(X = Y, Y = X).
commute(X < Y, Y > X).
commute(X <= Y, Y >= X).
commute(X > Y, Y < X).
commute(X >= Y, Y <= X).
commute(X # Y, Y # X).

/*
1.2 Relation Schemas

*/
relation(staedte, [sname, bev, plz, vorwahl, kennzeichen]).
relation(plz, [plz, ort]).
relation(ten, [no]).
relation(thousand, [no]).

spelling(staedte:plz, pLZ).
spelling(staedte:sname, sName).
spelling(plz, lc(plz)).
spelling(plz:plz, pLZ).
spelling(ten, lc(ten)).
spelling(ten:no, lc(no)).
spelling(thousand, lc(thousand)).
spelling(thousand:no, lc(no)).

hasIndex(Rel, attr(_:A, _, _), IndexName) :-
  hasIndex(Rel, attr(A, _, _), IndexName).

hasIndex(rel(plz, _, _), attr(ort, _, _), plz_ind).
hasIndex(rel(plz, _, _), attr(pLZ, _, _), plz_plz).











