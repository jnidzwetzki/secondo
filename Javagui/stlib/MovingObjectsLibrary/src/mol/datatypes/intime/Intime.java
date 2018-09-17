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

package mol.datatypes.intime;

import mol.datatypes.GeneralType;
import mol.datatypes.time.TimeInstant;

/**
 * Class for representation of the 'Intime' data type.<br>
 * Stores a value of type {@code <E>} and an 'TimeInstant' object
 * 
 * @author Markus Fuessel
 *
 * @param <E>
 *           - type of the value of a 'Intime' object
 */
public class Intime<E extends GeneralType> extends GeneralType implements Comparable<Intime<E>> {

   /**
    * The TimeInstant value
    */
   private TimeInstant instant;

   /**
    * The value of type {@code <E>} of this 'Intime' object
    */
   private E value;

   /**
    * Simple constructor, creates an undefined 'Intime' object
    */
   public Intime() {
   }

   /**
    * Simple constructor, creates an 'Intime' object with the passed values
    * 
    * @param instant
    * @param value
    */
   public Intime(final TimeInstant instant, final E value) {
      this.instant = instant;
      this.value = value;

      setDefined(instant.isDefined() && value.isDefined());
   }

   /**
    * Get the 'TimeInstant'
    * 
    * @return the instant
    */
   public TimeInstant getInstant() {
      return instant;
   }

   /**
    * Get the value of type {@code <E>}
    * 
    * @return the value
    */
   public E getValue() {
      return value;
   }

   /**
    * Intime objects where ordered/compared by their 'Instant'<br>
    * Note: this class has a natural ordering that is inconsistent with equals.
    *
    * @see java.lang.Comparable#compareTo(java.lang.Object)
    * 
    * @param otherIntimeObject
    *           - the other 'Intime' object to compare
    * @return a negative, zero, or a positive integer as the 'Instant' of this
    *         object is less than, equal to, or greater than the 'Instant' of the
    *         passed object.
    */
   @Override
   public int compareTo(final Intime<E> otherIntimeObject) {
      return instant.compareTo(otherIntimeObject.getInstant());
   }

}
