
//----------------------------------------------------
// The following code was generated by CUP v0.10k
// Mon Aug 17 20:01:11 CEST 2015
//----------------------------------------------------

package mmdb.streamprocessing.parser;

import java_cup10.runtime.*;
import java.util.Stack;
import java.util.ArrayList;
import java.io.FileInputStream;
import java.io.StringReader;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import mmdb.streamprocessing.parser.nestedlist.*;

/** CUP v0.10k generated parser.
  * @version Mon Aug 17 20:01:11 CEST 2015
  */
public class parser extends java_cup10.runtime.lr_parser {

  /** Default constructor. */
  public parser() {super();}

  /** Constructor which sets the default scanner. */
  public parser(java_cup10.runtime.Scanner s) {super(s);}

  /** Production table. */
  protected static final short _production_table[][] = 
    unpackFromStrings(new String[] {
    "\000\014\000\002\003\003\000\002\002\004\000\002\003" +
    "\003\000\002\004\005\000\002\004\004\000\002\006\003" +
    "\000\002\006\004\000\002\005\003\000\002\005\003\000" +
    "\002\005\003\000\002\005\003\000\002\005\003" });

  /** Access to production table. */
  public short[][] production_table() {return _production_table;}

  /** Parse-action table. */
  protected static final short[][] _action_table = 
    unpackFromStrings(new String[] {
    "\000\020\000\016\004\007\006\004\007\014\010\010\011" +
    "\006\012\005\001\002\000\022\002\ufffa\004\ufffa\005\ufffa" +
    "\006\ufffa\007\ufffa\010\ufffa\011\ufffa\012\ufffa\001\002\000" +
    "\022\002\ufff7\004\ufff7\005\ufff7\006\ufff7\007\ufff7\010\ufff7" +
    "\011\ufff7\012\ufff7\001\002\000\022\002\ufff6\004\ufff6\005" +
    "\ufff6\006\ufff6\007\ufff6\010\ufff6\011\ufff6\012\ufff6\001\002" +
    "\000\020\004\007\005\017\006\004\007\014\010\010\011" +
    "\006\012\005\001\002\000\022\002\ufff8\004\ufff8\005\ufff8" +
    "\006\ufff8\007\ufff8\010\ufff8\011\ufff8\012\ufff8\001\002\000" +
    "\004\002\015\001\002\000\022\002\uffff\004\uffff\005\uffff" +
    "\006\uffff\007\uffff\010\uffff\011\uffff\012\uffff\001\002\000" +
    "\022\002\001\004\001\005\001\006\001\007\001\010\001" +
    "\011\001\012\001\001\002\000\022\002\ufff9\004\ufff9\005" +
    "\ufff9\006\ufff9\007\ufff9\010\ufff9\011\ufff9\012\ufff9\001\002" +
    "\000\004\002\000\001\002\000\020\004\ufffc\005\ufffc\006" +
    "\ufffc\007\ufffc\010\ufffc\011\ufffc\012\ufffc\001\002\000\022" +
    "\002\ufffd\004\ufffd\005\ufffd\006\ufffd\007\ufffd\010\ufffd\011" +
    "\ufffd\012\ufffd\001\002\000\020\004\007\005\022\006\004" +
    "\007\014\010\010\011\006\012\005\001\002\000\020\004" +
    "\ufffb\005\ufffb\006\ufffb\007\ufffb\010\ufffb\011\ufffb\012\ufffb" +
    "\001\002\000\022\002\ufffe\004\ufffe\005\ufffe\006\ufffe\007" +
    "\ufffe\010\ufffe\011\ufffe\012\ufffe\001\002" });

  /** Access to parse-action table. */
  public short[][] action_table() {return _action_table;}

  /** <code>reduce_goto</code> table. */
  protected static final short[][] _reduce_table = 
    unpackFromStrings(new String[] {
    "\000\020\000\010\003\010\004\012\005\011\001\001\000" +
    "\002\001\001\000\002\001\001\000\002\001\001\000\012" +
    "\003\015\004\012\005\011\006\017\001\001\000\002\001" +
    "\001\000\002\001\001\000\002\001\001\000\002\001\001" +
    "\000\002\001\001\000\002\001\001\000\002\001\001\000" +
    "\002\001\001\000\010\003\020\004\012\005\011\001\001" +
    "\000\002\001\001\000\002\001\001" });

  /** Access to <code>reduce_goto</code> table. */
  public short[][] reduce_table() {return _reduce_table;}

  /** Instance of action encapsulation class. */
  protected CUP$parser$actions action_obj;

  /** Action encapsulation object initializer. */
  protected void init_actions()
    {
      action_obj = new CUP$parser$actions(this);
    }

  /** Invoke a user supplied parse action. */
  public java_cup10.runtime.Symbol do_action(
    int                        act_num,
    java_cup10.runtime.lr_parser parser,
    java.util.Stack            stack,
    int                        top)
    throws java.lang.Exception
  {
    /* call code in generated class */
    return action_obj.CUP$parser$do_action(act_num, parser, stack, top);
  }

  /** Indicates start state. */
  public int start_state() {return 0;}
  /** Indicates start production. */
  public int start_production() {return 1;}

  /** <code>EOF</code> Symbol index. */
  public int EOF_sym() {return 0;}

  /** <code>error</code> Symbol index. */
  public int error_sym() {return 1;}


	
	public parser(String parseString) {
		super();
		Yylex yyl = new Yylex(new StringReader(parseString)); 
		setScanner(yyl);
	}
	
	public NestedListNode parseToNestedList() throws Exception {
		Symbol s = parse();
		return (NestedListNode) s.value;
	}

}

/** Cup generated class to encapsulate user supplied action code.*/
class CUP$parser$actions {


	public String removeQuotes(String quotedString) {
		int length = quotedString.length();
		if(length >= 3) {
			if(quotedString.charAt(length-1) == '"') {
				quotedString = quotedString.substring(0,length-1);
			}
			if(quotedString.charAt(0) == '"') {
				quotedString = quotedString.substring(1);
			}
			
		}
		return quotedString;		
	}

  private final parser parser;

  /** Constructor */
  CUP$parser$actions(parser parser) {
    this.parser = parser;
  }

  /** Method with the actual generated action code. */
  public final java_cup10.runtime.Symbol CUP$parser$do_action(
    int                        CUP$parser$act_num,
    java_cup10.runtime.lr_parser CUP$parser$parser,
    java.util.Stack            CUP$parser$stack,
    int                        CUP$parser$top)
    throws java.lang.Exception
    {
      /* Symbol object for return from actions */
      java_cup10.runtime.Symbol CUP$parser$result;

      /* select the action based on the action number */
      switch (CUP$parser$act_num)
        {
          /*. . . . . . . . . . . . . . . . . . . .*/
          case 11: // atom ::= SYMBOL 
            {
              NestedListNode RESULT = null;
		int sleft = ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).left;
		int sright = ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).right;
		String s = (String)((java_cup10.runtime.Symbol) CUP$parser$stack.elementAt(CUP$parser$top-0)).value;
		
				RESULT = new SymbolAtom(s);
				
              CUP$parser$result = new java_cup10.runtime.Symbol(3/*atom*/, ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).left, ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).right, RESULT);
            }
          return CUP$parser$result;

          /*. . . . . . . . . . . . . . . . . . . .*/
          case 10: // atom ::= BOOL 
            {
              NestedListNode RESULT = null;
		int bleft = ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).left;
		int bright = ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).right;
		Boolean b = (Boolean)((java_cup10.runtime.Symbol) CUP$parser$stack.elementAt(CUP$parser$top-0)).value;
		
				RESULT = new BooleanAtom(b);
				
              CUP$parser$result = new java_cup10.runtime.Symbol(3/*atom*/, ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).left, ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).right, RESULT);
            }
          return CUP$parser$result;

          /*. . . . . . . . . . . . . . . . . . . .*/
          case 9: // atom ::= STRING 
            {
              NestedListNode RESULT = null;
		int sleft = ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).left;
		int sright = ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).right;
		String s = (String)((java_cup10.runtime.Symbol) CUP$parser$stack.elementAt(CUP$parser$top-0)).value;
		
				RESULT = new StringAtom(removeQuotes(s));
				
              CUP$parser$result = new java_cup10.runtime.Symbol(3/*atom*/, ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).left, ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).right, RESULT);
            }
          return CUP$parser$result;

          /*. . . . . . . . . . . . . . . . . . . .*/
          case 8: // atom ::= REAL 
            {
              NestedListNode RESULT = null;
		int rleft = ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).left;
		int rright = ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).right;
		Float r = (Float)((java_cup10.runtime.Symbol) CUP$parser$stack.elementAt(CUP$parser$top-0)).value;
		
				RESULT = new RealAtom(r);
				
              CUP$parser$result = new java_cup10.runtime.Symbol(3/*atom*/, ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).left, ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).right, RESULT);
            }
          return CUP$parser$result;

          /*. . . . . . . . . . . . . . . . . . . .*/
          case 7: // atom ::= INT 
            {
              NestedListNode RESULT = null;
		int ileft = ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).left;
		int iright = ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).right;
		Integer i = (Integer)((java_cup10.runtime.Symbol) CUP$parser$stack.elementAt(CUP$parser$top-0)).value;
		
				RESULT = new IntegerAtom(i);
				
              CUP$parser$result = new java_cup10.runtime.Symbol(3/*atom*/, ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).left, ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).right, RESULT);
            }
          return CUP$parser$result;

          /*. . . . . . . . . . . . . . . . . . . .*/
          case 6: // s_exp_list ::= s_exp_list s_exp 
            {
              ArrayListWrapper RESULT = null;
		int selleft = ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-1)).left;
		int selright = ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-1)).right;
		ArrayListWrapper sel = (ArrayListWrapper)((java_cup10.runtime.Symbol) CUP$parser$stack.elementAt(CUP$parser$top-1)).value;
		int seleft = ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).left;
		int seright = ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).right;
		NestedListNode se = (NestedListNode)((java_cup10.runtime.Symbol) CUP$parser$stack.elementAt(CUP$parser$top-0)).value;
		
				sel.add(se);
				RESULT = sel;
				
              CUP$parser$result = new java_cup10.runtime.Symbol(4/*s_exp_list*/, ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-1)).left, ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).right, RESULT);
            }
          return CUP$parser$result;

          /*. . . . . . . . . . . . . . . . . . . .*/
          case 5: // s_exp_list ::= s_exp 
            {
              ArrayListWrapper RESULT = null;
		int seleft = ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).left;
		int seright = ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).right;
		NestedListNode se = (NestedListNode)((java_cup10.runtime.Symbol) CUP$parser$stack.elementAt(CUP$parser$top-0)).value;
		
				ArrayListWrapper list = new ArrayListWrapper();
				list.add(se);
				RESULT = list;
				
              CUP$parser$result = new java_cup10.runtime.Symbol(4/*s_exp_list*/, ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).left, ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).right, RESULT);
            }
          return CUP$parser$result;

          /*. . . . . . . . . . . . . . . . . . . .*/
          case 4: // list ::= OPEN CLOSE 
            {
              NestedListNode RESULT = null;
		
				RESULT = new ListNode(new ArrayListWrapper());
				
              CUP$parser$result = new java_cup10.runtime.Symbol(2/*list*/, ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-1)).left, ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).right, RESULT);
            }
          return CUP$parser$result;

          /*. . . . . . . . . . . . . . . . . . . .*/
          case 3: // list ::= OPEN s_exp_list CLOSE 
            {
              NestedListNode RESULT = null;
		int selleft = ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-1)).left;
		int selright = ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-1)).right;
		ArrayListWrapper sel = (ArrayListWrapper)((java_cup10.runtime.Symbol) CUP$parser$stack.elementAt(CUP$parser$top-1)).value;
		
				ListNode interNode = new ListNode(sel);
				RESULT = interNode;
				
              CUP$parser$result = new java_cup10.runtime.Symbol(2/*list*/, ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-2)).left, ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).right, RESULT);
            }
          return CUP$parser$result;

          /*. . . . . . . . . . . . . . . . . . . .*/
          case 2: // s_exp ::= atom 
            {
              NestedListNode RESULT = null;
		int aleft = ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).left;
		int aright = ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).right;
		NestedListNode a = (NestedListNode)((java_cup10.runtime.Symbol) CUP$parser$stack.elementAt(CUP$parser$top-0)).value;
		 
				RESULT = a;
				
              CUP$parser$result = new java_cup10.runtime.Symbol(1/*s_exp*/, ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).left, ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).right, RESULT);
            }
          return CUP$parser$result;

          /*. . . . . . . . . . . . . . . . . . . .*/
          case 1: // $START ::= s_exp EOF 
            {
              Object RESULT = null;
		int start_valleft = ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-1)).left;
		int start_valright = ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-1)).right;
		NestedListNode start_val = (NestedListNode)((java_cup10.runtime.Symbol) CUP$parser$stack.elementAt(CUP$parser$top-1)).value;
		RESULT = start_val;
              CUP$parser$result = new java_cup10.runtime.Symbol(0/*$START*/, ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-1)).left, ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).right, RESULT);
            }
          /* ACCEPT */
          CUP$parser$parser.done_parsing();
          return CUP$parser$result;

          /*. . . . . . . . . . . . . . . . . . . .*/
          case 0: // s_exp ::= list 
            {
              NestedListNode RESULT = null;
		int lleft = ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).left;
		int lright = ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).right;
		NestedListNode l = (NestedListNode)((java_cup10.runtime.Symbol) CUP$parser$stack.elementAt(CUP$parser$top-0)).value;
		
				RESULT = l;
				
              CUP$parser$result = new java_cup10.runtime.Symbol(1/*s_exp*/, ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).left, ((java_cup10.runtime.Symbol)CUP$parser$stack.elementAt(CUP$parser$top-0)).right, RESULT);
            }
          return CUP$parser$result;

          /* . . . . . .*/
          default:
            throw new Exception(
               "Invalid action number found in internal parse table");

        }
    }
}

