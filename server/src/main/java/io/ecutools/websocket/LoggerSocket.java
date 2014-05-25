package io.ecutools.websocket;

import java.util.ArrayList;

import org.eclipse.jetty.websocket.api.Session;
import org.eclipse.jetty.websocket.api.annotations.OnWebSocketClose;
import org.eclipse.jetty.websocket.api.annotations.OnWebSocketConnect;
import org.eclipse.jetty.websocket.api.annotations.OnWebSocketError;
import org.eclipse.jetty.websocket.api.annotations.OnWebSocketFrame;
import org.eclipse.jetty.websocket.api.annotations.OnWebSocketMessage;
import org.eclipse.jetty.websocket.api.annotations.WebSocket;
import org.eclipse.jetty.websocket.api.extensions.Frame;

@WebSocket
public class LoggerSocket {

	private static ArrayList<Session> clients = new ArrayList<Session>(10);

	@OnWebSocketConnect
	public void onConnect(Session session) {
		clients.add(session);
		System.out.printf("LoggerSocket connection from %s\n", session.getRemoteAddress().getHostString());
	}

	@OnWebSocketMessage
    public void onText(Session session, String message) {
		System.out.printf("LoggerSocket received message [%s]\n", message);
		if(session.isOpen()) {
			for(Session client : clients) {
				System.out.println(String.format("Sending client %s text %s", session.getRemoteAddress().getAddress().toString(), message));
				if(session.getRemoteAddress().equals(client.getRemoteAddress())) continue;
				client.getRemote().sendString(message, null);
			}
        }
    }

	@OnWebSocketClose
	public void onClose(Session session, int code, String reason) {
		clients.remove(session);
		System.out.printf("LoggerSocket closed. host=%s, code=%d, reason=%s\n", session.getRemoteAddress().getHostString(), code, reason);
	}

	@OnWebSocketError
	public void onError(Session session, Throwable error) {
		System.out.printf("LoggerSocket error. host=%s, error=%s\n", session.getRemoteAddress().getHostString(), error.getMessage());
	}

	@OnWebSocketFrame
	public void onFrame(Session session, Frame frame) {
		System.out.printf("LoggerSocket frame. host=%s, frame=%s\n", session.getRemoteAddress().getHostString(), frame.toString());
	}
}