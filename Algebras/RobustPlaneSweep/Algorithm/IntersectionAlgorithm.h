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

#include <vector>
#include <memory>
#include <limits.h>

#include "../Helper/IntersectionAlgorithmData.h"
#include "../Helper/InternalGeometries.h"
#ifdef RPS_TEST
#include "../Helper/SpatialAlgebraStubs.h"
#else
#include "SpatialAlgebra.h"
#endif

namespace RobustPlaneSweep
{
class IntersectionAlgorithm
{
private:
  InternalPointTransformation* _transformation;
  IntersectionAlgorithmData* _data;
  bool _firstGeometryIsRegion;
  bool _secondGeometryIsRegion;

  static int
  OverlappingSegmentsSortComparer(const InternalResultLineSegment& x,
                                  const InternalResultLineSegment& y);

  static bool OverlappingSegmentsSortLess(const InternalResultLineSegment& x,
                                          const InternalResultLineSegment& y)
  {
    return OverlappingSegmentsSortComparer(x, y) < 0;
  }

protected:
  std::vector<InternalResultLineSegment>*
  RemoveOverlappingSegments(std::vector<InternalResultLineSegment>& segments);

  void CreateTransformation();

  explicit IntersectionAlgorithm(IntersectionAlgorithmData* data)
  {
    _data = data;
    _firstGeometryIsRegion = _data->FirstGeometryIsRegion();
    _secondGeometryIsRegion = _data->SecondGeometryIsRegion();
    _transformation = NULL;
  }

  virtual ~IntersectionAlgorithm()
  {
    if (_transformation != NULL) {
      delete _transformation;
      _transformation = NULL;
    }
  }

  virtual int GetInitialScaleFactor() const = 0;

  inline InternalPointTransformation* GetTransformation() const
  {
    return _transformation;
  }

  inline IntersectionAlgorithmData* GetData() const
  {
    return _data;
  }

  inline bool FirstGeometryIsRegion() const
  {
    return _firstGeometryIsRegion;
  }

  inline bool SecondGeometryIsRegion() const
  {
    return _secondGeometryIsRegion;
  }

public:
  virtual void DetermineIntersections() = 0;

  // This method is only useful for test cases. Do not use otherwise!
  inline void SetTransformation(InternalPointTransformation *transformation)
  {
    _transformation = new InternalPointTransformation(*transformation);
  }
};
}
