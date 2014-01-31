package vertx;

import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

import org.vertx.java.core.Handler;
import org.vertx.java.core.buffer.Buffer;
import org.vertx.java.core.http.HttpServerRequest;
import org.vertx.java.core.http.ServerWebSocket;
import org.vertx.java.platform.Verticle;

public class WebSocketVerticle extends Verticle {

	  //private static Map<String, Session> sessions;
	  private static Map<String, ServerWebSocket> connectedClients;

	  public WebSocketVerticle() {
		  //sessions = new HashMap<String, Session>(10);
		  connectedClients = new HashMap<String, ServerWebSocket>(10);
	  }

	  public void start() {
		  vertx.createHttpServer().websocketHandler(new Handler<ServerWebSocket>() {
			  public void handle(final ServerWebSocket ws) {

				  container.logger().info("Connection request to: " + ws.path() + ", from: " + ws.remoteAddress());

				  if(ws.path().contains("/ecutune")) {
					  onConnect(ws);
					  ws.dataHandler(new Handler<Buffer>() {
						  @Override
						  public void handle(Buffer data) {
							  onMessage(ws, data.toString());
						  }
					  });
					  ws.closeHandler(new Handler<Void>() {
						  @Override
						  public void handle(Void v) {
							  connectedClients.remove(ws.textHandlerID());
							  container.logger().info("Closing client connection: " + ws.textHandlerID());
						  }
					  });
				  } else {
					  ws.reject();
				  }
			  }
		  }).requestHandler(new Handler<HttpServerRequest>() {
			  public void handle(HttpServerRequest req) {
				  if (req.path().equals("/")) req.response().sendFile("websockets/ws.html");
			  }
	    }).listen(8080);
	  }

	  /**
	   * Handles new connection from a client
	   * 
	   * @param ws
	   */
	  public void onConnect(ServerWebSocket ws) {
		  String clientId = ws.textHandlerID();
		  container.logger().info("New connection: " + clientId);
		  connectedClients.put(clientId, ws);
		  while(!ws.headers().iterator().hasNext()) {
			  container.logger().info(String.format("Parsed Header: %s", ws.headers().iterator().next()));
		  }
	  }

	  /**
	   * Handles a message from a client
	   * 
	   * @param ws
	   * @param message
	   */
	  public void onMessage(ServerWebSocket ws, String message) {
		  String clientTextHandlerId = ws.textHandlerID();
		  message = message.trim();
		  container.logger().info("Received message: " + message.length() + " bytes from client: " + clientTextHandlerId);
		  broadcast(ws, message, clientTextHandlerId);
	  }

	  /**
	   * Broadcasts a message to all connected clients
	   * 
	   * @param ws
	   * @param message
	   */
	  public void broadcast(ServerWebSocket ws, String message, String senderTextHandlerId) {
		  for(Entry<String, ServerWebSocket> client : connectedClients.entrySet()) {
			  String clientTextHandlerId = client.getValue().textHandlerID();
			  if(clientTextHandlerId == senderTextHandlerId) continue;
			  container.logger().info("Broadcasting " + message.length() + " bytes to client: " + clientTextHandlerId);
		      ServerWebSocket clientSocket = client.getValue();
		      clientSocket.writeTextFrame(message);
		  }
	  }
}