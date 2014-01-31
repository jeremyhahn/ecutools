package vertx;

import java.util.UUID;

import org.testng.Assert;
import org.testng.annotations.Test;

public class MessageTests {

	@Test
	public void serializationAndDeserializationWorks() {
		
		String sessionId = UUID.randomUUID().toString();
		
		Message message = new Message();
		message.setAppId("abc123");
		message.setAppKey("123456");
		message.setCommand("rpm 1000");
		message.setSessionId(sessionId);

		Assert.assertEquals(message.getAppId(), "abc123");
		Assert.assertEquals(message.getAppKey(), "123456");
		Assert.assertEquals(message.getCommand(), "rpm 1000");
		Assert.assertEquals(message.getSessionId(), sessionId);

		message.setAppId("testme");
		Assert.assertEquals(message.getAppId(), "testme");
		Assert.assertEquals(message.getAppKey(), "123456");
		Assert.assertEquals(message.getCommand(), "rpm 1000");
		Assert.assertEquals(message.getSessionId(), sessionId);
	}
}