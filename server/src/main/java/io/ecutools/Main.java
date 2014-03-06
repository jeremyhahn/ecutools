package io.ecutools;

// http://www.eclipse.org/jetty/documentation/current/jetty-maven-helloworld.html

import io.ecutools.LoggerServlet;

import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.ServerConnector;
import org.eclipse.jetty.servlet.ServletContextHandler;
import org.eclipse.jetty.servlet.ServletHolder;
 
public class Main {

    public static void main(String[] args) throws Exception {
        
    	Server server = new Server();
        ServerConnector connector = new ServerConnector(server);
        connector.setPort(8080);
        server.addConnector(connector);

        ServletContextHandler context = new ServletContextHandler(ServletContextHandler.SESSIONS);
        context.setContextPath("/");
        context.addServlet(new ServletHolder(LoggerServlet.class), "/ecutune");
        server.setHandler(context);

        try {
            server.start();
            server.dumpStdErr();
            server.join();
        }
        catch (Throwable t) {
            t.printStackTrace(System.err);
        }
    }
}