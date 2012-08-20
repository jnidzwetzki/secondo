package viewer.queryconstruction;

/*
 * This code is based on an example provided by John Vella,
 * a tutorial reader.
 */

import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.util.ArrayList;
import java.util.*;
import javax.swing.*;
import sj.lang.ListExpr;
import viewer.QueryconstructionViewer;

/* ObjectsPane.java requires no other files. */
public class ObjectPane  extends JComponent implements MouseListener {
    
    private ArrayList<ObjectView> elements = new ArrayList<ObjectView>();
    private ArrayList<ObjectView> attributeElements = new ArrayList<ObjectView>();
    private QueryconstructionViewer viewer;
    private ListExpr objects;
    private JTextField textfield = new JTextField();
    private InfoDialog dialog = new InfoDialog();

    public ObjectPane (QueryconstructionViewer viewer) {
        this.viewer = viewer;
        this.setLayout(new GridLayout(1, 0));
        
        ActionListener textL = new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                String objName = textfield.getText();
                updateObjects(objName);
            }
        };
        
        textfield.addActionListener(textL);
    }
    
    //add all relations and objects of berlintest database to the viewer
    public void addObjects(ListExpr list){
        this.removeAll();
        elements.clear();
        this.add(textfield);
        
        // the length must be two and the object element must be a symbol atom with content "inquiry"
        if(list.listLength()==2 && list.first().symbolValue().equals("inquiry")) { 
            objects = list.second().second();
            
            while (objects.listLength() > 1) {
                ListExpr object = objects.second();
                
                ObjectView objectView = new ObjectView(object);
                if (!objectView.getName().startsWith("SEC")) {
                    addObject(objectView);
                }
                objects = objects.rest();
            }
        }
        this.update();
    }
    
    public void addObjects(ArrayList<ObjectView> objects) {
        this.elements = objects;
    }
    
    public ArrayList<ObjectView> getObjects(){
        return this.elements;
    }
    
    /**
     * adds the object to the object panel if it does not exist already
     * @param object 
     */
    protected void addObject(ObjectView object){
        object.addMouseListener(this);
        object.setActive(true);
        add(object);
        this.elements.add(object);
    }
    
    /**
     * adds the object to the object panel if it does not exist already
     * @param object 
     */
    protected void addAttributeObject(ObjectView object){
        object.addMouseListener(this);
        object.setActive(true);
        add(object);
        this.attributeElements.add(object);
    }
    
    protected void clear() {
        this.attributeElements.clear();
    }
    
    // updates the panel, only active objects are shown
    public void update() {
        setPreferredSize(new Dimension (this.getComponentCount()*120, 70));
        this.revalidate();
    }
    
    public void showAllObjects() {
        this.removeAll();
        this.add(textfield);
        for ( Iterator iter = attributeElements.iterator(); iter.hasNext(); ) {
            ObjectView object = (ObjectView)iter.next();
            this.add(object);
            object.setActive(true);
        }
        for ( Iterator iter = elements.iterator(); iter.hasNext(); ) {
            ObjectView object = (ObjectView)iter.next();
            this.add(object);
            object.setActive(true);
        }
        update();
    }
    
    // updates the panel, only active that fit to the input of the textfield are shown
    public void updateObjects(String type) {
        String[] types = type.split(",");
        for ( Iterator iter = elements.iterator(); iter.hasNext(); ) {
            ObjectView object = (ObjectView)iter.next();
            object.setActive(false);
            for (String s: types) {
                if (s.equals("bool"))
                    return;
                if (object.getName().toLowerCase().startsWith(s.toLowerCase()) || object.getType().equals(s)) {
                    object.setActive(true);
                }
            }
            if (object.isActive()) {
                this.add(object);
            }
            else {
                this.remove(object);
            }
        }
        this.update();
    }

    public void mouseClicked ( MouseEvent arg0 ) {
        if (arg0.getComponent() instanceof ObjectView) {
            ObjectView element = (ObjectView)arg0.getComponent();
            
            if (arg0.getButton() == 3) {
                String name = element.getName() + "("+viewer.getCount(element.getName()).trim()+")";
                //generating the info dialog of the clicked object
                if (element.getOType() == null) {
                    dialog.viewInfo(name, element.getType());
                }
                else {
                    dialog.viewInfo(name, element.getOType().getViewString());
                }
                dialog.view();
            }
            else {
                //a copy of the object is added to the main panel
                ObjectView new_object = new ObjectView(element.getType(), element.getName());
                new_object.setOType(element.getOType());
                viewer.addObject(new_object);
            }
        }
    }
    public void mouseReleased(MouseEvent e) {}
    public void mouseEntered(MouseEvent e){}
    public void mouseExited(MouseEvent e){}
    public void mousePressed(MouseEvent e) {}

}
