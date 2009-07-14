
import java.util.*;
import org.w3c.dom.*;
import javax.xml.parsers.*;
import java.io.*;
import java.util.zip.*;
import java.util.regex.Pattern;


public class SecondoExtension{

   protected boolean valid;
   protected int secondo_Major_Version = -1;     // version informtion
   protected int secondo_Minor_Version = 0;      // version information
   protected int secondo_SubMinor_Version = 0;   // version information
   protected Vector<StringPair> files = new Vector<StringPair>();
   protected String copyright=null;

   boolean filesPresent(ZipFile f, Vector<String> names){
     for(int i=0;i<names.size();i++){
        if(f.getEntry(names.get(i))==null){
          System.err.println("Entry " + names.get(i) + " not found");
          return false;
        }
     }
     return  true;
   }

   public boolean isValid(){
     return valid;
   }

   protected boolean readCopyright(Node n){
      if(!n.hasChildNodes()){
        return false;
      }
      String cr = n.getFirstChild().getNodeValue().trim();
      if(cr.length()==0){
        System.err.println("empty copyright file found");
        return false;
      }
      copyright = cr;
      return true;
   }


  /** Extracts the version information from the xml file **/
  boolean readSecondoVersion(Node n1){
    NodeList nl = n1.getChildNodes();
    for(int i=0;i<nl.getLength(); i++){
       Node n = nl.item(i);
       String name = n.getNodeName();
       if(name.equals("Major")){
          if(n.hasChildNodes()){
             String  a = n.getFirstChild().getNodeValue().trim();
             secondo_Major_Version=Integer.parseInt(a.trim());
          } 
       } else if(name.equals("Minor")){
          if(n.hasChildNodes()){
             String  a = n.getFirstChild().getNodeValue().trim();
             secondo_Minor_Version=Integer.parseInt(a.trim());
          } 
       } else if(name.equals("SubMinor")){
          if(n.hasChildNodes()){
             String  a = n.getFirstChild().getNodeValue().trim();
             secondo_SubMinor_Version=Integer.parseInt(a.trim());
          } 
       } else if(!name.equals("#text") && !name.equals("#comment")){
           System.err.println("Unknown version information found" + name);
       }
    }
    return secondo_Major_Version>0 &&
           secondo_Minor_Version>=0 &&
           secondo_SubMinor_Version>=0;
  }

  static boolean copyZipEntryToFile(File f, ZipFile zip, ZipEntry e){
     boolean ok = true;
     File path = f.getParentFile();
     if(!path.exists()){
       path.mkdirs();
     }
     InputStream in = null;
     OutputStream out = null;
     try{
       in = zip.getInputStream(e);
       byte[] buffer = new byte[1024];
       out = new FileOutputStream(f);
       int read = 0;
       int size = 0;
       while((read=in.read(buffer))>=0){
         size += read;
         out.write(buffer,0,read);
       }

     } catch(Exception ex){
       System.err.println("Problem in copying file " + f);
       ok = false;
     } finally{
         if(in!=null){
           try{in.close();} catch(Exception ex){ System.err.println("Problem in closing in file");}
         }
         if(out!=null){
           try{out.close();} catch(Exception ex){System.out.println("Problen in closing out file");}
         }
 
     }
     return ok;
  }


   /** Reads the required files and its location **/
   protected boolean readFiles(Node n1){
     NodeList nl = n1.getChildNodes();
     for(int i=0;i<nl.getLength();i++){
       Node n = nl.item(i);
       String name = n.getNodeName();
       if(!name.equals("File") && !name.equals("#text") && !name.equals("#comment")){
         System.err.println("Unknown node name for files detected: " + name);
       } else if(name.equals("File")){
          StringPair pair = new StringPair();
         // get the filename
         if(n.hasChildNodes()){
            String fn = n.getFirstChild().getNodeValue().trim();
            if(fn.length()>0){
               pair.first = fn;
            } 
         }
         if(pair.first==null){
            System.err.println("XMLFile corrupt: filename missing");
            return false;
         }
         // get the location
         NamedNodeMap m = n.getAttributes();
         Node loc  = m.getNamedItem("location");
         if(loc==null){
            System.err.println("XML-file corupted: location of a file is missing");
            return false;
         }
         String tmp = loc.getNodeValue().trim();
         while(tmp.startsWith("/")){
              tmp = tmp.substring(1,tmp.length()-1);
         }
         while(tmp.endsWith("/")){
             tmp = tmp.substring(0,tmp.length()-1);
         }
         if(tmp.length()==0){
            System.err.println("invalid value for location");
            return false;
         }
         pair.second = tmp;
         files.add(pair);  
       }
     }
     return true;
   } 


   /** extracts the secondo version from the existing system. 
     * If any error occurs, the result will be null.
     **/
   public static Version readSecondoVersion(String secondoDir){
       String s = File.separator;
       File versionFile = new File(secondoDir + s + "include" + s + "version.h");
       if(!versionFile.exists()){
         System.err.println("Version file '"+versionFile.getAbsolutePath()+"' not found");
         return null;
       }
       int major = -1;
       int minor = -1;
       int subminor = -1;
       try{
          BufferedReader in = new BufferedReader(new FileReader(versionFile));
          while((major<0 || minor <0 || subminor <0) && in.ready()){
             String line = in.readLine();
             if(line!=null){
                line = line.trim();
                if(line.indexOf("SECONDO_VERSION_MAJOR") >=0){
                    line = line.replaceAll("[^0123456789]",""); // only keep digits
                    major = Integer.parseInt(line); 
                } else if(line.indexOf("SECONDO_VERSION_MINOR") >=0){
                    line = line.replaceAll("[^0123456789]",""); // only keep digits
                    minor = Integer.parseInt(line); 
                } else if(line.indexOf("SECONDO_VERSION_REVISION") >=0){
                    line = line.replaceAll("[^0123456789]",""); // only keep digits
                    subminor = Integer.parseInt(line); 
                }
             }
          }
          in.close();
          if(major<0 || minor < 0  || subminor <0){
             System.err.println("version not completely found");
             return null;
          }
       } catch (Exception e){
         e.printStackTrace();
         return null;
       }
      Version res = new Version();
      res.major = major;
      res.minor = minor;
      res.subminor = subminor;
      return res;
   }


}
