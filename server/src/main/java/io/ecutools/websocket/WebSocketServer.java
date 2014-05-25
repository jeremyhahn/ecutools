package io.ecutools.websocket;

import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.ServerConnector;
import org.eclipse.jetty.servlet.ServletContextHandler;
import org.eclipse.jetty.servlet.ServletHolder;

public class WebSocketServer {

	private static Server server;

	public static void start() {

		server = new Server();
        ServerConnector connector = new ServerConnector(server);
        connector.setPort(8080);
        server.addConnector(connector);

        ServletContextHandler context = new ServletContextHandler(ServletContextHandler.SESSIONS);
        context.setContextPath("/");
        context.addServlet(new ServletHolder(LoggerServlet.class), "/logger");
        //context.addServlet(new ServletHolder(TunerServlet.class), "/tuner");
        server.setHandler(context);

        try {
            server.start();
            server.dumpStdErr();
            server.join();
        }
        catch(Throwable t) {
            t.printStackTrace(System.err);
        }
	}

	public static void stop() {
		try {
			server.stop();
		}
		catch (Exception e) {
			e.printStackTrace();
		}
	}
}
