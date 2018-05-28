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

package mol.datatypes.time;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.time.format.DateTimeFormatter;

import org.junit.BeforeClass;
import org.junit.Test;

import mol.datatypes.base.BaseInt;

/**
 * Tests for class 'TimeInstant'
 * 
 * @author Markus Fuessel
 */
public class TimeInstantTest {

   @BeforeClass
   public static void setUpBeforeClass() throws Exception {
      DateTimeFormatter format = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss:SSS");

      TimeInstant.setDefaultDateTimeFormat(format);
   }

   @Test
   public void testCompareTo_EqualTimeInstant_ShouldBeZero() {

      TimeInstant t1 = new TimeInstant("2018-05-01 20:35:30:123");
      TimeInstant t2 = new TimeInstant("2018-05-01 20:35:30:123");

      assertTrue(t1.compareTo(t2) == 0);

   }

   @Test
   public void testCompareTo_LowerVsGreaterTimeInstant_ShouldBeLowerZero() {

      TimeInstant t1 = new TimeInstant("2018-05-01 20:35:30:122");
      TimeInstant t2 = new TimeInstant("2018-05-01 20:35:30:123");

      assertTrue(t1.compareTo(t2) < 0);

   }

   @Test
   public void testCompareTo_GreaterVsLowerTimeInstant_ShouldBeLowerZero() {

      TimeInstant t1 = new TimeInstant("2018-05-01 20:35:30:124");
      TimeInstant t2 = new TimeInstant("2018-05-01 20:35:30:123");

      assertTrue(t1.compareTo(t2) > 0);

   }

   @Test
   public void testEquals_EqualTimeInstants_ShouldBeTrue() {
      TimeInstant t1 = new TimeInstant("2018-05-01 20:35:30:123");
      TimeInstant t2 = new TimeInstant("2018-05-01 20:35:30:123");

      assertTrue(t1.equals(t2));
   }

   @Test
   public void testEquals_NonEqualTimeInstants_ShouldBeFalse() {
      TimeInstant t1 = new TimeInstant("2018-05-01 20:35:30:123");
      TimeInstant t2 = new TimeInstant("2018-05-01 20:35:30:124");

      assertFalse(t1.equals(t2));
   }

   @Test
   public void testPlusMillis_AddMillisecondToTimeInstant() {
      TimeInstant t1 = new TimeInstant("2018-05-01 20:35:30:123").plusMillis(1);
      TimeInstant t2 = new TimeInstant("2018-05-01 20:35:30:124");

      assertTrue(t1.equals(t2));
      assertTrue(t2.compareTo(t2.plusMillis(1)) < 0);
   }

   @Test
   public void testMinusMillis_SubtractMillisecondFromTimeInstant() {
      TimeInstant t1 = new TimeInstant("2018-05-01 20:35:30:123");
      TimeInstant t2 = new TimeInstant("2018-05-01 20:35:30:124").minusMillis(1);

      assertTrue(t1.equals(t2));
      assertTrue(t1.compareTo(t1.minusMillis(1)) > 0);
   }

   @Test
   public void testPlusNanos_AddNanosecondToTimeInstant() {
      TimeInstant t = new TimeInstant("2018-05-01 20:35:30:124");

      assertTrue(t.compareTo(t.plusNanos(1)) < 0);
   }

   @Test
   public void testMinusNanos_SubtractNanosecondFromTimeInstant() {
      TimeInstant t = new TimeInstant("2018-05-01 20:35:30:124");

      assertTrue(t.compareTo(t.minusNanos(1)) > 0);
   }

   @Test
   public void testBefore_LowerVsHigherInstant_ShouldBeTrue() {
      TimeInstant t = new TimeInstant("2018-05-01 20:35:30:124");

      assertTrue(t.before(t.plusMillis(1)));
      assertTrue(t.before(t.plusNanos(1)));

   }

   @Test
   public void testBefore_EqualInstants_ShouldBeFalse() {
      TimeInstant t1 = new TimeInstant("2018-05-01 20:35:30:124");
      TimeInstant t2 = new TimeInstant("2018-05-01 20:35:30:124");

      assertFalse(t1.before(t2));

   }

   @Test
   public void testAfter_HigherVsLowerInstant_ShouldBeTrue() {
      TimeInstant t = new TimeInstant("2018-05-01 20:35:30:124");

      assertTrue(t.after(t.minusMillis(1)));
      assertTrue(t.after(t.minusNanos(1)));

   }

   @Test
   public void testAfter_EqualInstants_ShouldBeFalse() {
      TimeInstant t1 = new TimeInstant("2018-05-01 20:35:30:124");
      TimeInstant t2 = new TimeInstant("2018-05-01 20:35:30:124");

      assertFalse(t1.after(t2));

   }

   @Test
   public void testAdjacent_AdjacentInstants_ShouldBeTrue() {
      TimeInstant t = new TimeInstant("2018-05-01 20:35:30:124");

      assertTrue(t.adjacent(t.plusNanos(1)));
      assertTrue(t.adjacent(t.minusNanos(1)));

   }

   @Test
   public void testAdjacent_NonAdjacentOrEqualInstants_ShouldBeFalse() {
      TimeInstant t1 = new TimeInstant("2018-05-01 20:35:30:124");
      TimeInstant t2 = new TimeInstant("2018-05-01 20:35:30:124");

      assertFalse(t1.adjacent(t2));
      assertFalse(t1.adjacent(t1.plusMillis(1)));

   }
}