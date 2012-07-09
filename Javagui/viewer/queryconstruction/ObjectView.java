/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package viewer.queryconstruction;

import java.awt.Color;
import java.awt.Graphics;
import javax.swing.ImageIcon;
import javax.swing.JComponent;
import javax.swing.JLabel;
import sj.lang.ListExpr;
import viewer.QueryconstructionViewer;

/**
 *
 * @author lrentergent
 */
public class ObjectView extends JComponent {
    
    private String name;
    private String label;
    private int xpos;
    private int ypos;
    private ObjectType otype;
    private String type;
    private boolean active = false;
    private Color color = Color.BLACK;
    
    public ObjectView(String type, String name){
        
        this.name = name;
        this.label = name;
        this.type = type;
        
    }
    
    public ObjectView(ListExpr list){
        //generate an instance of ObjectType
        otype = new ObjectType(list);
        
        this.name = otype.getName();
        this.type = otype.getType();
        this.label = this.name;
        
    }
    
    /** paints a Secondo ObjectView into the RelationsPane
     height 80, width 50*/
    public void paintComponent(Graphics g, int x, int y, Color color){
        this.xpos = 10 + x*120;
        this.ypos = 10 + y*40;
        
        g.setColor(this.color);
        
        if (type.equals(ObjectType.OPERATION)) {
            g.drawOval(xpos, ypos, 90, 50);
        }
        else {
            g.drawRect(xpos, ypos, 90, 50);
            
            if (type.equals(ObjectType.RELATION)) {
                ImageIcon icon = new ImageIcon(QueryconstructionViewer.class.getResource("queryconstruction/images/relation.gif"));
                g.drawImage(icon.getImage(), xpos + 5, ypos + 5, this);
            }

            if (type.equals(ObjectType.MPOINT)) {
                ImageIcon icon = new ImageIcon(QueryconstructionViewer.class.getResource("queryconstruction/images/mpoint.gif"));
                g.drawImage(icon.getImage(), xpos + 5, ypos + 5, this);
            }
            
            if (type.equals(ObjectType.POINT)) {
                ImageIcon icon = new ImageIcon(QueryconstructionViewer.class.getResource("queryconstruction/images/point.gif"));
                g.drawImage(icon.getImage(), xpos + 5, ypos + 5, this);
            }
            
            if (type.equals(ObjectType.REGION)) {
                ImageIcon icon = new ImageIcon(QueryconstructionViewer.class.getResource("queryconstruction/images/region.gif"));
                g.drawImage(icon.getImage(), xpos + 5, ypos + 5, this);
            }
            
            if (type.equals(ObjectType.MREGION)) {
                ImageIcon icon = new ImageIcon(QueryconstructionViewer.class.getResource("queryconstruction/images/mregion.gif"));
                g.drawImage(icon.getImage(), xpos + 5, ypos + 5, this);
            }
        }
        
        int w = g.getFontMetrics().stringWidth(label);
        String s = label;
        if (w > 90) {
            s = label.substring(0, 10);
            w = g.getFontMetrics().stringWidth(s);
        }
        
        g.drawString(s, xpos + 45 - w/2, ypos + 30);
    }
    
    public void paintTable(Graphics g, int x, int y){
        this.xpos = 10 + x*120;
        this.ypos = 10 + y*30;
        
        g.setColor(this.color);
        
        g.drawLine(xpos - 10, ypos + 20, xpos + 120, ypos + 20);
        
        int w = g.getFontMetrics().stringWidth(label);
        String s = label;
        if (w > 80) {
            s = label.substring(0, 12);
        }
        
        g.drawString(s, xpos + 10, ypos + 10);
    }
    
    public void viewInfo(InfoDialog dialog) {
        //dialog.add(this);
        JLabel name = new JLabel( otype.getName(), JLabel.LEADING );
        JLabel type = new JLabel( otype.getType(), JLabel.LEADING );
        String[] attributes = otype.getAttributes();
        String[] attrtypes = otype.getAttrTypes();
        int i = 0;
        for (String att: attributes) {
            JLabel label = new JLabel( att, JLabel.LEADING );
            dialog.add(label);
            
            dialog.add(new JLabel( attrtypes[i] ));
            i++;
        }
        
        dialog.view();
    }
    
    public String getName() {
        return this.name;
    }
    
    public void setName(String name) {
        this.name = name;
        if (name.length() < 12) {
            label = name;
        }
    }
    
    public String getType(){
        return type;
    }
    
    public ObjectType getOType() {
        return otype;
    }
    
    public void setOType(ObjectType otype) {
        this.otype = otype;
    }
    
    public boolean isActive() {
        return active;
    }
    
    public void setActive() {
        color = Color.BLACK;
        active = true;
    }
    
    public void setUnactive() {
        color = Color.GRAY;
        active = false;
    }
}
