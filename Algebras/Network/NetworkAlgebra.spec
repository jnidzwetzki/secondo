operator length alias LENGTH pattern op ( _ )
operator no_components alias NO_COMPONENTS pattern op ( _ )
operator isempty alias ISEMPTY pattern op ( _ )
operator thenetwork alias THENETWORK pattern op ( _, _ )
operator routes alias ROUTES pattern op ( _ )
operator junctions alias JUNCTIONS pattern op ( _ )
operator sections alias SECTIONS pattern op ( _ )
operator inside alias INSIDE pattern _ infixop _
operator netdistance alias NETDISTANCE pattern op ( _ , _ )
operator netdistancenew alias NETDISTANCENEW pattern op ( _ , _ )
operator point2gpoint alias POINT2GPOINT pattern op ( _ , _ )
operator gpoint2point alias GPOINT2POINT pattern op ( _ )
operator = alias EQUAL pattern _ infixop _
operator line2gline alias LINE2GLINE pattern op ( _ , _ )
operator polygpoints alias POLYGPOINTS pattern op ( _ , _ )
operator routeintervals alias ROUTEINTERVALS pattern op ( _ )
operator intersects alias INTERSECTS pattern _ infixop _
operator gpoint2rect alias GPOINT2RECT pattern op ( _ )
operator gline2line alias GLINE2LINE pattern op ( _ )
operator union alias UNION pattern _ infixop _
operator distance alias DISTANCE pattern op ( _ , _ )
operator shortest_path alias SHORTEST_PATH pattern op ( _ , _ )
operator shortest_pathastar alias SHORTEST_PATHASTAR pattern op ( _ , _ )
operator getBGP alias GETBGP pattern op ( _ )
operator spsearchvisited alias SPSEARCHVISITED pattern op ( _ , _ , _ )
operator shortestpathtree alias SHORTESTPATHTREE pattern op ( _ , _ )
operator getAdjacentSections alias GETADJACENTSECTIONS pattern op ( _ , _ , _ )
operator getReverseAdjacentSections alias GETREVERSEADJACENTSECTIONS pattern op ( _ , _ , _ )
operator circlen alias CIRCLEN pattern op( _ , _ )
operator in_circlen alias IN_CIRCLEN pattern op( _ , _ )
operator out_circlen alias OUT_CIRCLEN pattern op( _ , _ )
operator netbbox alias NETBBOX pattern op( _ )
