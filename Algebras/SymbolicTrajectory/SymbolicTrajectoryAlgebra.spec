operator contains alias CONTAINS pattern _ infixop _
operator topattern alias TOPATTERN pattern _ op
operator matches alias MATCHES pattern _ infixop _
operator indexmatches alias INDEXMATCHES pattern _ op [ _ , _ , _ ]
operator filterMatches alias FILTERMATCHES pattern _ op [ _ , _ ]
operator rewrite alias REWRITE pattern _ _ op
operator classify alias CLASSIFY pattern _ _ op
operator indexclassify alias INDEXCLASSIFY pattern _ op [ _ , _ , _ ]
operator compress alias COMPRESS pattern op ( _ )
operator fillgaps alias FILLGAPS pattern op ( _ , _ )
operator createml alias CREATEML pattern op ( _ , _ )
operator createmlrelation alias CREATEMLRELATION pattern op ( _ , _ , _ )
operator createindex alias INDEX pattern op(_)
operator createrelindex alias CREATERELINDEX pattern _ op [ _ ]