package vertx;

import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

abstract public class SessionManager {

	private static Map<String, Session> sessions = new HashMap<String, Session>(10);

	public static void addSession(Session session) {
		sessions.put(session.getAccountId(), session);
	}

	public static Session getSession(String accountId) {
		for(Entry<String, Session> session : sessions.entrySet()) {
			  if(accountId.equals(session.getValue().getAccountId())) {
				  return session.getValue();
			  }
		}
		return null;
	}

	public static void remove(String sessionId) {
		for(Entry<String, Session> session : sessions.entrySet()) {
			  if(sessionId.equals(session.getValue().getAccountId())) {
				  sessions.remove(session.getKey());
			  }
		}
	}
}