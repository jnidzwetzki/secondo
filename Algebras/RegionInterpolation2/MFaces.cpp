/*
*/

#include "interpolate.h"

MFaces::MFaces() {
}

MFaces::MFaces(MFace face) {
    AddFace(face);
}

void MFaces::AddFace(MFace face) {
    faces.push_back(face);
}

MFaces MFaces::GetBorderRegions(vector<Reg> *regs) {
    MFaces ret;
    
    for (unsigned int i = 0; i < regs->size(); i++) {
        ret.AddFace(MFace((*regs)[i].GetMSegs()));
    }
    
    return ret;
}

URegion MFaces::ToURegion(Interval<Instant> iv, double start, double end) {
    vector<URegion> uregs;

    if (start == 0 && end == 1) {
        for (unsigned int i = 0; i < faces.size(); i++) {
            URegion u = faces[i].ToURegion(iv, i);
            uregs.push_back(u);
        }
    } else {
        MFaces f = divide(start, end);
        URegion ret = f.ToURegion(iv, 0, 1);

        return ret;
    }

    URegion ret(uregs[0]);
    for (unsigned int i = 1; i < uregs.size(); i++) {
        ret.AddURegion(&uregs[i]);
    }

    return ret;
}

ListExpr MFaces::ToListExpr(Interval<Instant> iv, double start, double end) {
    ListExpr le;

    if (start == 0 && end == 1) {
        ListExpr inst =
                nl->OneElemList(nl->StringAtom(iv.start.ToString(false), true));
        le = nl->Append(inst, nl->StringAtom(iv.end.ToString(false), true));
        le = nl->Append(le, nl->BoolAtom(iv.lc));
        le = nl->Append(le, nl->BoolAtom(iv.rc));
        ListExpr ur = nl->OneElemList(inst);
        
        ListExpr urs = nl->OneElemList(faces[0].ToListExpr());
        le = urs;
        for (unsigned int i = 1; i < faces.size(); i++) {
            le = nl->Append(le, faces[i].ToListExpr());
        }
        nl->Append(ur, urs);

        return ur;
    } else {
        MFaces f = divide(start, end);
        return f.ToListExpr(iv, 0, 1);
    }
}

MRegion MFaces::ToMRegion(Interval<Instant> _iv) {
    MRegion ret(1);
    bool needStartRegion = false, needEndRegion = false;
    Interval<Instant> iv;
    iv.CopyFrom(_iv);
    DateTime msec1(durationtype, 1);

    for (unsigned int i = 0; i < faces.size(); i++) {
        faces[i].MergeConcavities();
        needStartRegion = needStartRegion || faces[i].needStartRegion;
        needEndRegion = needEndRegion || faces[i].needEndRegion;
    }

    if (needStartRegion) {
        iv.lc = false;
        Interval<Instant> startiv(iv.start, iv.start + msec1, true, true);
        iv.start = iv.start + msec1;
        URegion start = GetBorderRegions(sregs).ToURegion(startiv, 0, 1);
        ret.AddURegion(start);
    }

    if (needEndRegion) {
        iv.rc = false;
        Interval<Instant> endiv(iv.end - msec1, iv.end, true, true);
        iv.end = iv.end - msec1;
        URegion end = GetBorderRegions(dregs).ToURegion(endiv, 0, 1);
        ret.AddURegion(end);
    }

    URegion ureg = ToURegion(iv, 0, 1);
    ret.AddURegion(ureg);

    return ret;
}

ListExpr MFaces::ToMListExpr(Interval<Instant> _iv) {
    bool needStartRegion = false, needEndRegion = false;
    Interval<Instant> iv;
    iv.CopyFrom(_iv);
    DateTime msec1(durationtype, 1);
    
    ListExpr mreg;
    ListExpr le;
    for (unsigned int i = 0; i < faces.size(); i++) {
        faces[i].MergeConcavities();
        needStartRegion = needStartRegion || faces[i].needStartRegion;
        needEndRegion = needEndRegion || faces[i].needEndRegion;
    }
    
    Interval<Instant> startiv(iv.start, iv.start + msec1, true, true);
    Interval<Instant> endiv(iv.end - msec1, iv.end, true, true);
    
    if (needStartRegion) {
        iv.lc = false;
        iv.start += msec1;
        mreg = nl->OneElemList(
                GetBorderRegions(sregs).ToListExpr(startiv, 0, 1));
    }
    
    if (needEndRegion) {
        iv.rc = false;
        iv.end -= msec1;
    }
    
    if (!needStartRegion)
        mreg = le = nl->OneElemList(ToListExpr(iv, 0, 1));
    else
        le = nl->Append(mreg, ToListExpr(iv, 0, 1));

    if (needEndRegion) {
        cerr << "\n\nCreating END REGION\n";
        nl->Append(le, GetBorderRegions(dregs).ToListExpr(endiv, 0, 1));
    }

    return mreg;
}

MFaces MFaces::divide(double start, double end) {
    MFaces ret;

    for (unsigned int i = 0; i < faces.size(); i++) {
        ret.AddFace(faces[i].divide(start, end));
    }

    return ret;
}

string MFaces::ToString() {
    std::ostringstream ss;

    ss << "\n"
            << "=========================  MFaces  ========================\n";

    for (unsigned int i = 0; i < faces.size(); i++) {
        ss << " === Face " << i << " ===\n";
        ss << faces[i].ToString();
    }

    ss << "=========================  /MFaces  ========================\n\n";

    return ss.str();
}