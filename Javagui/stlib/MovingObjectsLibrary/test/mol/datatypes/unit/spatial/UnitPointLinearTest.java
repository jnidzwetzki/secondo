//This file is part of SECONDO.

//Copyright (C) 2014, University in Hagen, Department of Computer Science,
//Database Systems for New Applications.

//SECONDO is free software; you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation; either version 2 of the License, or
//(at your option) any later version.

//SECONDO is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

//You should have received a copy of the GNU General Public License
//along with SECONDO; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
package mol.datatypes.unit.spatial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;

import java.time.format.DateTimeFormatter;

import org.junit.BeforeClass;
import org.junit.Test;

import mol.TestUtil.TestUtilData;
import mol.datatypes.interval.Period;
import mol.datatypes.spatial.Point;
import mol.datatypes.spatial.util.Rectangle;
import mol.datatypes.time.TimeInstant;

/**
 * Tests for the 'UnitPointLinear' class
 * 
 * @author Markus Fuessel
 */
public class UnitPointLinearTest {

   @BeforeClass
   public static void setUpBeforeClass() throws Exception {
      DateTimeFormatter format = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss:SSS");

      TimeInstant.setDefaultDateTimeFormat(format);
   }

   @Test
   public void testGetValue() {

      Point p0 = new Point(1.0d, 1.0d);
      Point p1 = new Point(10.0d, 10.0d);
      Point pExpected = new Point(5.0d, 5.0d);

      Period period = TestUtilData.getPeriod("01", "10", true, true);
      TimeInstant instant = TestUtilData.getInstant("05");

      UnitPoint upoint = new UnitPointLinear(period, p0, p1);

      Point pointAt = upoint.getValue(instant);

      assertEquals(pExpected, pointAt);
   }

   @Test
   public void testGetValue_AtValidInstant() {
      Period period = new Period("2018-01-01 00:00:00:000", "2018-01-10 00:00:00:000", true, true);
      Point pointStart = new Point(1.0d, 1.0d);
      Point pointEnd = new Point(10.0d, 10.0d);
      UnitPoint upoint = new UnitPointLinear(period, pointStart, pointEnd);

      TimeInstant instant1 = new TimeInstant("2018-01-01 00:00:00:000");
      TimeInstant instant2 = new TimeInstant("2018-01-05 00:00:00:000");
      TimeInstant instant3 = new TimeInstant("2018-01-10 00:00:00:000");

      assertEquals(new Point(1.0d, 1.0d), upoint.getValue(instant1));
      assertEquals(new Point(5.0d, 5.0d), upoint.getValue(instant2));
      assertEquals(new Point(10.0d, 10.0d), upoint.getValue(instant3));

   }

   @Test
   public void testGetValue_AtInvalidInstant_ShouldReturnUndefinedPoint() {
      Period period = new Period("2018-01-01 00:00:00:000", "2018-01-10 00:00:00:000", false, false);
      Point pointStart = new Point(1.0d, 1.0d);
      Point pointEnd = new Point(10.0d, 10.0d);
      UnitPoint upoint = new UnitPointLinear(period, pointStart, pointEnd);

      TimeInstant instant1 = new TimeInstant("2018-01-01 00:00:00:000");
      TimeInstant instant2 = new TimeInstant("2018-01-10 00:00:00:000");
      TimeInstant instant3 = new TimeInstant("2017-01-01 00:00:00:000");
      TimeInstant instant4 = new TimeInstant();

      Point point1 = upoint.getValue(instant1);
      Point point2 = upoint.getValue(instant2);
      Point point3 = upoint.getValue(instant3);
      Point point4 = upoint.getValue(instant4);

      assertFalse(point1.isDefined());
      assertFalse(point2.isDefined());
      assertFalse(point3.isDefined());
      assertFalse(point4.isDefined());

   }

   @Test
   public void testGetProjectionBoundingBox() {
      Period period = new Period("2018-01-01 00:00:00:000", "2018-01-10 00:00:00:000", true, true);
      Point pointStart = new Point(1.0d, 1.0d);
      Point pointEnd = new Point(10.0d, 10.0d);
      UnitPoint upoint = new UnitPointLinear(period, pointStart, pointEnd);

      Rectangle expectedPBB = new Rectangle(1.0d, 10.0d, 10.0d, 1.0d);

      assertEquals(expectedPBB, upoint.getProjectionBoundingBox());

   }

   @Test
   public void testAtPeriod_IntersectingPeriod() {
      Period period = new Period("2018-01-01 00:00:00:000", "2018-01-10 00:00:00:000", true, true);
      Point pointStart = new Point(0.0d, 0.0d);
      Point pointEnd = new Point(10.0d, 10.0d);
      UnitPoint oldUPoint = new UnitPointLinear(period, pointStart, pointEnd);

      Period newPeriod = new Period("2018-01-05 12:00:00:000", "2018-01-20 00:00:00:000", true, true);

      UnitPoint newUPoint = oldUPoint.atPeriod(newPeriod);

      assertEquals(period.intersection(newPeriod), newUPoint.getPeriod());
      assertEquals(new Point(5.0d, 5.0d), newUPoint.getInitial());
      assertEquals(pointEnd, newUPoint.getFinal());

   }

   @Test
   public void testAtPeriod_NoIntersectingPeriod() {
      Period period = new Period("2018-01-01 00:00:00:000", "2018-01-10 00:00:00:000", true, true);
      Point pointStart = new Point(0.0d, 0.0d);
      Point pointEnd = new Point(10.0d, 10.0d);
      UnitPoint oldUPoint = new UnitPointLinear(period, pointStart, pointEnd);

      Period newPeriod = new Period("2018-01-15 12:00:00:000", "2018-01-20 00:00:00:000", true, true);

      UnitPoint newUPoint = oldUPoint.atPeriod(newPeriod);

      assertFalse(newUPoint.isDefined());

   }

}
