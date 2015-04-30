package com.secondo.webgui.client.mainview;

import com.google.gwt.event.dom.client.ClickEvent;
import com.google.gwt.event.dom.client.ClickHandler;
import com.google.gwt.user.client.ui.Button;
import com.google.gwt.user.client.ui.HorizontalPanel;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.ListBox;
import com.google.gwt.user.client.ui.VerticalPanel;

/**
 * @author Irina Russkaya
 *
 */
public class SimpleQueryPanel extends VerticalPanel {
	private DefaultTextBox textForQuery;
	private Button queryButton;
	private Button countButton;
	private Label resultInfoLabel;
	private ListBox numberOfTrajectoriesToBeShown;
	private HorizontalPanel numberOfTrajectoriesToBeShownPanel;

	public SimpleQueryPanel(String textForHelpInfoLabel, String typeOfQuery,
			String defaultText) {
		super();

		this.setSpacing(4);
		this.setWidth("100%");
		Label helpInfoLabel = new Label(textForHelpInfoLabel);
		helpInfoLabel.setStylePrimaryName("labelForPasses");
		this.add(helpInfoLabel);

		if (typeOfQuery.equals("passThrough")) {
			Label helpInfoLevel2 = new Label(
					"Define with your mouse a region on the map");
			helpInfoLevel2.setStylePrimaryName("labelForPasses");
			this.add(helpInfoLevel2);
		} else {
			textForQuery = new DefaultTextBox(defaultText);
			textForQuery.setWidth("90%");
			this.add(textForQuery);
		}

		if (typeOfQuery.equals("passThrough") || typeOfQuery.equals("pass")) {
			HorizontalPanel panelForButtons = new HorizontalPanel();
			queryButton = new Button("retrieve");
			queryButton.setStyleName("floatRight");
			panelForButtons.add(queryButton);
			countButton = new Button("count");
			countButton.setStyleName("floatRight");
			panelForButtons.add(countButton);
			this.add(panelForButtons);

			numberOfTrajectoriesToBeShownPanel = new HorizontalPanel();
			Label numberOfTrajectoriesToShowBeforeLabel = new Label(
					"Show up to ");
			Label numberOfTrajectoriesToShowAfterLabel = new Label(
					"     trajectories");
			numberOfTrajectoriesToBeShown = new ListBox();
			numberOfTrajectoriesToBeShown.addItem(" ");
			numberOfTrajectoriesToBeShown.addItem("3");
			numberOfTrajectoriesToBeShown.addItem("5");
			numberOfTrajectoriesToBeShown.addItem("7");
			numberOfTrajectoriesToBeShownPanel
					.add(numberOfTrajectoriesToShowBeforeLabel);
			numberOfTrajectoriesToBeShownPanel
					.add(numberOfTrajectoriesToBeShown);
			numberOfTrajectoriesToBeShownPanel
					.add(numberOfTrajectoriesToShowAfterLabel);
			numberOfTrajectoriesToBeShownPanel.getElement().setAttribute(
					"cellpadding", "5px");
			numberOfTrajectoriesToBeShownPanel.getElement().setAttribute(
					"padding-left", "10px");
			numberOfTrajectoriesToBeShownPanel.getElement().setAttribute(
					"color", "#808080");
			numberOfTrajectoriesToBeShownPanel.setVisible(false);
			queryButton.addClickHandler(new ClickHandler() {
				@Override
				public void onClick(ClickEvent event) {
					numberOfTrajectoriesToBeShownPanel.setVisible(true);
				}
			});
			this.add(numberOfTrajectoriesToBeShownPanel);

			resultInfoLabel = new Label();
			this.add(resultInfoLabel);

		} else {
			queryButton = new Button("define");
			queryButton.setStyleName("floatRight");
			this.add(queryButton);
			resultInfoLabel = new Label();
			this.add(resultInfoLabel);
		}
	}

	/**
	 * Returns the user input used to build a query
	 * 
	 * @return The user input in a text box
	 */
	public DefaultTextBox getLabelTextForQuery() {
		return textForQuery;
	}

	/**
	 * Returns the button "retrieve"
	 * 
	 * @return The button "retrieve"
	 */
	public Button getQueryButton() {
		return queryButton;
	}

	/**
	 * Returns the label with result
	 * 
	 * @return The label with result
	 */
	public Label getResultInfoLabel() {
		return resultInfoLabel;
	}

	/**
	 * Returns the list box containing the number of trajectories to be shown
	 * 
	 * @return The number of trajectories to be shown
	 */
	public ListBox getNumberOfTrajectoriesToBeShown() {
		return numberOfTrajectoriesToBeShown;
	}

	/**
	 * Returns the button "count"
	 * 
	 * @return The button "count"
	 */
	public Button getCountButton() {
		return countButton;
	}

	/**
	 * Hides the option to select the number of trajectories to be shown
	 */
	public void hideNumberOfTrajectoriesToBeShownPanel() {
		numberOfTrajectoriesToBeShownPanel.setVisible(false);
	}

}
