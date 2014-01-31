package vertx;

import java.util.HashMap;
import java.util.Map.Entry;

import org.vertx.java.core.http.ServerWebSocket;

public class Session {

	private String accountId;
	private HashMap<String, ServerWebSocket> devices;

	public Session() {
		devices = new HashMap<String, ServerWebSocket>(10);
	}

	public String getAccountId() {
		return accountId;
	}

	public void setAccountId(String id) {
		this.accountId = id;
	}

	public HashMap<String, ServerWebSocket> getDevices() {
		return devices;
	}

	public ServerWebSocket getDevice(String deviceId) {
		for(Entry<String, ServerWebSocket> device : devices.entrySet()) {
			  String clientTextHandlerId = device.getValue().textHandlerID();
			  if(clientTextHandlerId == deviceId) {
				  return device.getValue();
			  }
		  }
		return null;
	}

	public void setDevices(HashMap<String, ServerWebSocket> devices) {
		this.devices = devices;
	}

	public boolean hasDevices() {
		return devices.size() > 0;
	}

	public void addDevice(String deviceId, ServerWebSocket websocket) {
		devices.put(deviceId, websocket);
	}

	@Override
	public String toString() {
		return "Session [accountId=" + accountId + ", devices=" + devices + "]";
	}
}