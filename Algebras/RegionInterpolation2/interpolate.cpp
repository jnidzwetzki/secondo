/*
*/

#include "MovingRegionAlgebra.h"
#include "SpatialAlgebra.h"
#include "GenOps.h"
#include "NestedList.h"
#include "ListUtils.h"
#include "DateTime.h"
#include "interpolate.h"

#include <string>

#ifdef USE_LUA
vector<pair<Face *, Face *> > _matchFacesLua(vector<Face> *src,
        vector<Face> *dst, int depth, string args);

static vector<pair<Face *, Face *> > matchFacesLua(vector<Face> *src,
        vector<Face> *dst, int depth, string args) {
    return _matchFacesLua(src, dst, depth, args);
}
#endif

static vector<pair<Face *, Face *> > matchFacesSimple(vector<Face> *src,
        vector<Face> *dst, int depth, string args) {
    vector<pair<Face *, Face *> > ret;

    for (unsigned int i = 0; (i < src->size() || (i < dst->size())); i++) {
        if ((i < src->size()) && (i < dst->size())) {
            pair<Face *, Face *> p(&((*src)[i]), &((*dst)[i]));
            ret.push_back(p);
        } else if (i < src->size()) {
            pair<Face *, Face *> p(&((*src)[i]), NULL);
            ret.push_back(p);
        } else {
            pair<Face *, Face *> p(NULL, &((*dst)[i]));
            ret.push_back(p);
        }
    }

    return ret;
}

static vector<pair<Face *, Face *> > matchFacesNull(vector<Face> *src,
        vector<Face> *dst, int depth, string args) {
    vector<pair<Face *, Face *> > ret;

    for (unsigned int i = 0; i < src->size(); i++) {
        pair<Face *, Face *> p(&((*src)[i]), NULL);
        ret.push_back(p);
    }
    for (unsigned int i = 0; i < dst->size(); i++) {
        pair<Face *, Face *> p(NULL, &((*dst)[i]));
        ret.push_back(p);
    }

    return ret;
}

static vector<pair<Face *, Face *> > matchFacesDistance(vector<Face> *src,
        vector<Face> *dst, int depth, string args) {
    vector<pair<Face *, Face *> > ret;

    for (unsigned int i = 0; i < src->size(); i++) {
        (*src)[i].used = 0;
    }
    for (unsigned int i = 0; i < dst->size(); i++) {
        (*dst)[i].used = 0;
    }

    Pt srcoff = Face::GetBoundingBox(*src).first;
    Pt dstoff = Face::GetBoundingBox(*dst).first;

    if (src->size() >= dst->size()) {
        for (unsigned int i = 0; i < dst->size(); i++) {
            int dist = 2000000000;
            int candidate = -1;
            for (unsigned int j = 0; j < src->size(); j++) {
                if ((*src)[j].used == 1)
                    continue;
                Pt srcm = (*src)[j].GetMiddle() - srcoff;
                Pt dstm = (*dst)[i].GetMiddle() - dstoff;
                int d2 = dstm.distance(srcm);
                if (d2 < dist) {
                    dist = d2;
                    candidate = j;
                }
            }
            pair<Face *, Face *> p(&(*src)[candidate], &(*dst)[i]);
            ret.push_back(p);
            (*src)[candidate].used = 1;
        }
        for (unsigned int j = 0; j < src->size(); j++) {
            if ((*src)[j].used)
                continue;
            pair<Face *, Face *> p(&(*src)[j], NULL);
            ret.push_back(p);
        }
    } else {
        for (unsigned int i = 0; i < src->size(); i++) {
            int dist = 2000000000;
            int candidate = -1;
            for (unsigned int j = 0; j < dst->size(); j++) {
                if ((*dst)[j].used == 1)
                    continue;
                Pt srcm = (*src)[i].GetMiddle() - srcoff;
                Pt dstm = (*dst)[j].GetMiddle() - dstoff;
                int d2 = dstm.distance(srcm);
                if (d2 < dist) {
                    dist = d2;
                    candidate = j;
                }
            }
            pair<Face *, Face *> p(&(*src)[i], &(*dst)[candidate]);
            ret.push_back(p);
            (*dst)[candidate].used = 1;
        }
        for (unsigned int j = 0; j < dst->size(); j++) {
            if ((*dst)[j].used)
                continue;
            pair<Face *, Face *> p(NULL, &(*dst)[j]);
            ret.push_back(p);
        }

    }

    return ret;
}

static bool sortLowerLeft(const Face& r1, const Face& r2) {
    return r1.v[0].s < r2.v[0].s;
}

static vector<pair<Face *, Face *> > matchFacesLowerLeft(vector<Face> *src,
        vector<Face> *dst, int depth, string args) {
    vector<pair<Face *, Face *> > ret;

    for (unsigned int i = 0; i < src->size(); i++) {
        (*src)[i].used = 0;
        (*src)[i].Close();
        if (!(*src)[i].v.size())
            (*src)[i].used = 1;
    }
    for (unsigned int i = 0; i < dst->size(); i++) {
        (*dst)[i].used = 0;
        (*dst)[i].Close();
        if (!(*dst)[i].v.size())
            (*dst)[i].used = 1;
    }

    std::sort(src->begin(), src->end(), sortLowerLeft);
    std::sort(dst->begin(), dst->end(), sortLowerLeft);


    unsigned int i = 0, j = 0;
    while (i < src->size() && j < dst->size()) {
        Pt p1 = (*src)[i].v[0].s;
        Pt p2 = (*dst)[j].v[0].s;

        if (p1 == p2) {
            (*src)[i].used = 1;
            (*dst)[j].used = 1;
            ret.push_back(pair<Face*, Face*>(&(*src)[i], &(*dst)[j]));
            i++;
            j++;
        } else if (p1 < p2) {
            i++;
        } else {
            j++;
        }
    }


//    for (unsigned int i = 0; i < src->size(); i++) {
//        for (unsigned int j = 0; j < dst->size(); j++) {
//            if ((*dst)[j].used)
//                continue;
//          cerr << "Comparing " << (*src)[i].v[0].s.ToString() << " with " <<
//                    (*dst)[j].v[0].s.ToString() << "\n";
//            if ((*src)[i].v[0].s == (*dst)[j].v[0].s) {
//                (*src)[i].used = 1;
//                (*dst)[j].used = 1;
//                ret.push_back(pair<Reg*, Reg*>(&(*src)[i], &(*dst)[j]));
//            }
//        }
//    }

    //    for (unsigned int i = 0; i < src->size(); i++) {
    //        if (!(*src)[i].used) {
    //            ret.push_back(pair<Reg*, Reg*>(&(*src)[i], NULL));
    //        }
    //    }
    //
    //    for (unsigned int i = 0; i < dst->size(); i++) {
    //        if (!(*dst)[i].used) {
    //            ret.push_back(pair<Reg*, Reg*>(NULL, &(*dst)[i]));
    //        }
    //    }

    return ret;
}

static vector<pair<Face *, Face *> > matchFaces(
       vector<Face> *src, vector<Face> *dst, int depth,
       vector<pair<Face*,Face*> > (*fn)(vector<Face>*,vector<Face>*,int,string),
       string args) {
    vector<pair<Face *, Face *> > ret;

    for (unsigned int i = 0; i < src->size(); i++) {
        (*src)[i].used = 0;
        (*src)[i].isdst = 0;
    }
    for (unsigned int i = 0; i < dst->size(); i++) {
        (*dst)[i].used = 0;
        (*dst)[i].isdst = 1;
    }

    vector<pair<Face *, Face *> > pairs = fn(src, dst, depth, args);

    for (unsigned int i = 0; i < src->size(); i++) {
        (*src)[i].used = 0;
    }
    for (unsigned int i = 0; i < dst->size(); i++) {
        (*dst)[i].used = 0;
    }

    for (unsigned int i = 0; i < pairs.size(); i++) {
        if (!pairs[i].first || !pairs[i].second || 
                pairs[i].first->used || pairs[i].second->used ||
                pairs[i].first->isdst || !pairs[i].second->isdst)
            continue;
        pairs[i].first->used = 1;
        pairs[i].second->used = 1;
        ret.push_back(pairs[i]);
    }

    for (unsigned int i = 0; i < src->size(); i++) {
        if (!(*src)[i].used) {
            ret.push_back(pair<Face*, Face*>(&(*src)[i], NULL));
        }
    }

    for (unsigned int i = 0; i < dst->size(); i++) {
        if (!(*dst)[i].used) {
            ret.push_back(pair<Face*, Face*>(NULL, &(*dst)[i]));
        }
    }

    return ret;
}

void handleIntersections(MFaces& children, MFace parent, bool evap, bool rs);

vector<pair<Face*,Face*> > (*matchingStrategy)(vector<Face>*,vector<Face>*,
        int,string);

/*
2 Interpolate

Main interpolation function between two lists of regions.
It calls a matching-function to create pairs of regions from the source- and
destination-list and interpolates the convex hulls of these regions using the
RotatingPlane-function. Then it creates two new lists from the concavities and
holes of the region and recurses. The result of the recursion is then merged
into the current result. Intersections are detected and tried to be compensated

*/

MFaces interpolate(vector<Face> *sregs, vector<Face> *dregs, int depth,
        bool evap, string args) {
    MFaces ret;

    ret.sregs = sregs;
    ret.dregs = dregs;

    if (sregs->empty() && dregs->empty()) // Nothing to do!
        return ret;

    // Prepare the regions
    for (unsigned int i = 0; i < sregs->size(); i++) {
        (*sregs)[i].isdst = 0;
        if ((*sregs)[i].v.size() < 3) {
            sregs->erase(sregs->begin() + i);
            i--;
        }
    }
    for (unsigned int i = 0; i < dregs->size(); i++) {
        (*dregs)[i].isdst = 1;
        if ((*dregs)[i].v.size() < 3) {
            dregs->erase(dregs->begin() + i);
            i--;
        }
    }
    
    
    // Match the faces to pairs of faces in the source- and destination-realm
    vector<pair<Face *, Face *> > matches;
    if (!evap)
        matches = matchFaces(sregs, dregs, depth, matchingStrategy, args);
    else
        matches = matchFaces(sregs, dregs, depth, matchFacesLowerLeft, args);
    
 
    for (unsigned int i = 0; i < matches.size(); i++) {
        pair<Face *, Face *> p = matches[i];

        Face *src = p.first;
        Face *dst = p.second;
        if (src && dst) {
 
            // Use the RotatingPlane-Algorithm to create an interpolation of
            // the convex hull of src and dst and identify all concavities
            RotatingPlane rp(src, dst, depth, evap);

            // Recurse and try to match and interpolate the list of concavities
            MFaces fcs = interpolate(&rp.scvs, &rp.dcvs, depth+1, evap, args);

            // Now check if the interpolations intersect in any way
            handleIntersections(fcs, rp.mface, evap, false);
            handleIntersections(fcs, rp.mface, false, true);

            ret.needSEvap = ret.needSEvap || fcs.needSEvap;
            ret.needDEvap = ret.needDEvap || fcs.needDEvap;

            for (unsigned int i = 0; i < fcs.faces.size(); i++) {
                if (fcs.faces[i].face.ignore)
                    continue;
                rp.mface.AddMsegs(fcs.faces[i].face);
                for (unsigned int j = 0; j < fcs.faces[i].holes.size(); j++) {
                    MFace fc(fcs.faces[i].holes[j]);
                    ret.AddFace(fc);
                }
            }

            rp.mface.MergeConcavities();
            ret.AddFace(rp.mface);
        } else {
            Face *r = src ? src : dst;
            MFace coll = r->collapseWithHoles(r == src);
            ret.AddFace(coll);
        }
    }

    // Toplevel-Intersections are still not handled yet, do that now.
    if (depth == 0) {
        handleIntersections(ret, MFace(), evap, false);
        handleIntersections(ret, MFace(), evap, true);
    }

    return ret;
}

void handleIntersections(MFaces& children, MFace parent, bool evap, bool rs) {
    vector<MSegs> evp;
    
    for (int i = 0; i < (int) children.faces.size(); i++) {
        children.faces[i].face.calculateBBox();
    }
    parent.face.calculateBBox();

    for (int i = 0; i < (int) children.faces.size(); i++) {
        MSegs *s1 = &children.faces[i].face;
//        cerr << "Checking i = " << i << "\n" << s1->ToString() << "\n";
        for (int j = 0; j <= i; j++) {
//            cerr << "Checking " << i << "/" << j << " total " <<
//            children.faces.size() << "\n";
            MSegs *s2 = (j == 0) ? &parent.face : &children.faces[j - 1].face;

            if (s1->intersects(*s2, false, false)) {
                assert(!restart);
                pair<MSegs, MSegs> ss;
                if (!s1->iscollapsed && !evap) {
                    ss = s1->kill();
                    children.faces.erase(children.faces.begin()+i);
                } else if (!s2->iscollapsed && (s2 != &parent.face)
                        && !evap) {
                    ss = s2->kill();
                    children.faces.erase(children.faces.begin() + (j-1));
                } else {
                    MSegs *rm;
                    if (evap) {
                        if (!(s1->iscollapsed || s2->iscollapsed)) {
                            cerr << "Intersection " << s1->ToString() << "\n"
                                 << s2->ToString() << "\n";
                        }
                        assert(s1->iscollapsed || s2->iscollapsed);
                        if (s1->iscollapsed)
                            rm = s1;
                        else
                            rm = s2;
                        vector<MSegs> ms;
                        if (rm->iscollapsed == 1) {
                            ms = rm->sreg.Evaporate(true);
                        } else {
                            ms = rm->dreg.Evaporate(false);
                        }
                        evp.insert(evp.end(), ms.begin(), ms.end());
                        cerr << "Intersection found, evaporating\n";
                    } else {
                        rm = s1;
                        if (rm->iscollapsed == 1)
                            children.needSEvap = true;
                        else
                            children.needDEvap = true;
                        cerr << "Intersection found, but cannot "
                                "compensate! Eliminating Region\n";
//                        cerr << rm->ToString() << "\n";
                    }
                    children.faces.erase(children.faces.begin()+
                                         (rm == s1 ? i : j - 1));
                    i--;
                    break;
                }
                cerr << "Intersection found, breaking up connection\n";
                children.faces.push_back(ss.first);
                children.faces.push_back(ss.second);
                i-=2;
                break;
            }
        }
    }

    children.faces.insert(children.faces.end(), evp.begin(), evp.end());
}


Word InMRegion(const ListExpr typeInfo,
        const ListExpr instance,
        const int errorPos,
        ListExpr& errorInfo,
        bool& correct);


#define USE_LISTS 1

int interpolate2valmap(Word* args,
        Word& result,
        int message,
        Word& local,
        Supplier s) {
    result = qp->ResultStorage(s);

    Instant* ti1 = static_cast<Instant*> (args[1].addr);
    Instant* ti2 = static_cast<Instant*> (args[3].addr);
    CcString* arg = static_cast<CcString*> (args[4].addr);
    MRegion* m = static_cast<MRegion*> (result.addr);

    Interval<Instant> iv(*ti1, *ti2, true, true);

    ListExpr _sregs = OutRegion(nl->Empty(), args[0]);
    ListExpr _dregs = OutRegion(nl->Empty(), args[2]);

    vector<Face> sregs = Face::getFaces(_sregs);
    vector<Face> dregs = Face::getFaces(_dregs);

#ifdef USE_LUA
    matchingStrategy = matchFacesLua;
#else
    matchingStrategy = matchFacesDistance;
#endif
    
    MFaces mf = interpolate(&sregs, &dregs, 0, false, arg->GetValue());

#ifdef USE_LISTS
    ListExpr mreg = mf.ToMListExpr(iv);

    ListExpr err;
    bool correct = false;
    Word w = InMRegion(nl->Empty(), mreg, 0, err, correct);
    
    if (correct)
        result.setAddr(w.addr);
    else {
        mreg = MFaces::fallback(&sregs, &dregs, iv);
        //        Word w = InMRegion(nl->Empty(), mreg, 0, err, correct);
        if (correct)
            result.setAddr(w.addr);
        else {
            MRegion mr = mf.ToMRegion(iv);
            *m = mr;
        }
    }
#else    
    MRegion mr = mf.ToMRegion(iv);
    *m = mr;
#endif

    return 0;
}

