package io.ecutools;

import org.eclipse.jetty.websocket.api.Session;
import org.eclipse.jetty.websocket.api.annotations.OnWebSocketClose;
import org.eclipse.jetty.websocket.api.annotations.OnWebSocketConnect;
import org.eclipse.jetty.websocket.api.annotations.OnWebSocketError;
import org.eclipse.jetty.websocket.api.annotations.OnWebSocketMessage;
import org.eclipse.jetty.websocket.api.annotations.WebSocket;

@WebSocket
public class LoggerSocket {

	@OnWebSocketConnect
	public void onConnect(Session session) {
		System.out.printf("LoggerSocket connection from %s\n", session.getRemoteAddress().getHostString());
	}

	@OnWebSocketMessage
    public void onText(Session session, String message) {
		System.out.printf("LoggerSocket received message [%s]\n", message);
		if(session.isOpen()) {
           session.getRemote().sendString(message, null);
        }
    }

	@OnWebSocketClose
	public void onClose(Session session, int code, String reason) {
		System.out.printf("LoggerSocket closed. host=%s, code=%d, reason=%s\n", session.getRemoteAddress().getHostString(), code, reason);
	}

	@OnWebSocketError
	public void onError(Session session, Throwable error) {
		System.out.printf("LoggerSocket error. host=%s, error=%s\n", session.getRemoteAddress().getHostString(), error.getMessage());
	}

	//@OnWebSocketFrame
	//public void onFrame(Session session, Frame frame) {
	//	System.out.printf("LoggerSocket host=%s, frame=%s", session.getRemoteAddress().getHostString(), frame.toString());
	//}
}