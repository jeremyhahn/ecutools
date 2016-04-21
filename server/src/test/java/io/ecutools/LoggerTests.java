package io.ecutools;

import io.ecutools.websocket.LoggerSocket;

import java.net.URI;
import java.util.concurrent.Future;

import org.eclipse.jetty.websocket.api.Session;
import org.eclipse.jetty.websocket.client.WebSocketClient;

public class LoggerTests {

    public static void main(String[] args) {

        URI uri = URI.create("ws://localhost:8080/logger/");
        WebSocketClient client = new WebSocketClient();
        try {
            try {
                client.start();
                LoggerSocket socket = new LoggerSocket();
                Future<Session> future = client.connect(socket,uri);
                Session session = future.get();
                session.getRemote().sendString("Test!");
                session.close();
            }
            finally {
                client.stop();
            }
        }
        catch (Throwable t) {
            t.printStackTrace(System.err);
        }
    }
}