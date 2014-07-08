package viewer.hoese;

import sj.lang.ListExpr;
import viewer.*;
import viewer.hoese.*;
import java.util.*;
import javax.swing.*;
import java.awt.*;
import tools.Reporter;

/**
 * This class computes strings that are necessary for displaying symbolic
 * trajectories.
 */
public class SymbolicValues {

  /**
   * Takes an interval and returns its start instant as a string. In the non-
   * standard case (i.e., rightclosed or not leftclosed), the leftclosed
   * indicator is prepended.
   */
  public static String intervalStartToString(Interval iv) {
    String result = "";
    if (!iv.isLeftclosed() || iv.isRightclosed()) {// non-standard
      result += (iv.isLeftclosed() ? "[" : "(");
    }
    else {
      result += " ";
    }
    return result + LEUtils.convertTimeToString(iv.getStart());  
  }
  
  /**
   * Takes an interval and returns its end instant as a string, starting only
   * at the time detail that differs from the start instant. In the non-standard
   * case, the rightclosed indicator is appended.
   */
  public static String intervalEndToString(Interval iv) {
    String result = "";
    String startStr = LEUtils.convertTimeToString(iv.getStart());
    String endStr = LEUtils.convertTimeToString(iv.getEnd());
    boolean print = false;
    for (int pos = 0; pos < startStr.length(); pos++) {
      if (print) {
        result += endStr.charAt(pos);
      }
      else if (!print && (startStr.charAt(pos) == endStr.charAt(pos))) {
        result += " ";
      }
      else { // first inequality
        while ((startStr.charAt(pos) >= '0') && (startStr.charAt(pos) <= '9')) {
          pos--;
        }
        result = result.substring(0, pos + 1);
        print = true;
      }
    }
    if (!iv.isLeftclosed() || iv.isRightclosed()) {// non-standard
      result += (iv.isRightclosed() ? "]" : ")");
    }
    return " " + result;
  }
  
  public static String labelToString(ListExpr label) {
    if (label.atomType() != ListExpr.TEXT_ATOM) {
      return "ERROR: wrong label type";
    }
    return label.textValue();
  }
  
  public static String labelsToString(ListExpr labels) {
    String result = "{";
    while (!labels.isEmpty()) {
      ListExpr label = labels.first();
      if (label.atomType() != ListExpr.TEXT_ATOM) {
        return "ERROR: wrong label type";
      }
      labels = labels.rest();
      result += label.textValue() + (!labels.isEmpty() ? ", " : "}");
    }
    return result;
  }
  
  public static String placeToString(ListExpr place) {
    if (place.listLength() != 2) {
      return "ERROR: wrong list size for a place";
    }
    String result = labelToString(place.first());
    if (place.second().atomType() != ListExpr.INT_ATOM) {
      return "ERROR: wrong place reference type";
    }
    return result + " : " + place.second().intValue();
  }
  
  public static String placesToString(ListExpr places) {
    String result = "{";
    while (!places.isEmpty()) {
      result += placeToString(places.first());
      places = places.rest();
      result += (!places.isEmpty() ? ", " : "}");
    }
    return result;
  }
}