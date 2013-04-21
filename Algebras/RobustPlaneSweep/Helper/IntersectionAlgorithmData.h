/*
----
This file is part of SECONDO.

Copyright (C) 2004, University in Hagen, Department of Computer Science,
Database Systems for New Applications.

SECONDO is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

SECONDO is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with SECONDO; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
----

*/

#pragma once

#ifdef RPS_TEST
#include "../Helper/SpatialAlgebraStubs.h"
#else
#include "SpatialAlgebra.h"
#endif

namespace RobustPlaneSweep
{
  typedef int HalfSegmentIntersectionId;

  class InternalAttribute;

  enum IntersectionAlgorithmCalculationType
  {
    CalulationTypeNone = 0,
    CalulationTypeLine = 1,
    CalulationTypeRegion = 2,
  };

  class IntersectionAlgorithmData
  {
  public:
    virtual IntersectionAlgorithmCalculationType GetCalculationType() = 0;

    virtual void InitializeFetch() = 0;

    virtual bool FetchInputHalfSegment(
      HalfSegment &segment,
      bool &belongsToSecondGeometry) = 0;

    virtual HalfSegmentIntersectionId GetHalfSegmentId(
      const HalfSegment& /*segment*/)
    {
      return 0;
    };

    virtual void OutputHalfSegment(
      const HalfSegment& segment,
      const InternalAttribute& attribute) = 0;

    virtual const Rectangle<2> GetBoundingBox() = 0;

    virtual bool IsInputOrderedByX() = 0;

    virtual void GetRoundToDecimals(int& decimals, int& stepSize) = 0;

    virtual void OutputFinished() = 0;

    virtual bool ReportIntersections()
    {
      return false;
    }

    virtual void ReportIntersection(
      const Point& /*intersectionPoint*/,
      const bool /*overlappingIntersection*/)
    {
    }

    virtual ~IntersectionAlgorithmData()
    {
    }
  };
}
