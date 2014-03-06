package io.ecutools;

import org.eclipse.jetty.websocket.servlet.WebSocketServlet;
import org.eclipse.jetty.websocket.servlet.WebSocketServletFactory;

@SuppressWarnings("serial")
public class LoggerServlet extends WebSocketServlet {

	@Override
    public void configure(WebSocketServletFactory factory) {
        factory.register(LoggerSocket.class);
    }
}