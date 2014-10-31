package com.secondo.webgui.utils.config;

import java.util.ResourceBundle;

public class ConfigManager {
	
	private static ConfigManager instance = null;

	public static ConfigManager getInstance() {
	if (instance == null) {
	instance = new ConfigManager();
	}
	return instance;
	}

	public static String getConfigString(String key) {
	return getInstance().getResources().getString(key);
	}

	private ResourceBundle resources;

	private ConfigManager() {
	resources = ResourceBundle.getBundle("config.web-conf");
	}

	public ResourceBundle getResources() {
	return resources;
	}



}
