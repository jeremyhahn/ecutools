package common;

import java.io.IOException;

import org.dozer.DozerBeanMapper;
import org.dozer.Mapper;

import com.fasterxml.jackson.annotation.JsonTypeInfo;
import com.fasterxml.jackson.databind.ObjectMapper;

@JsonTypeInfo(use=JsonTypeInfo.Id.CLASS, include=JsonTypeInfo.As.PROPERTY, property="@class")
public abstract class JsonObject<Type> {

	protected Class<? extends Type> clazz;
	protected ObjectMapper mapper;

	protected JsonObject(Class<? extends Type> clazz) {

		this.clazz = clazz;
		mapper = new ObjectMapper();
	}

	public String toJson() {

		String json = new String();

		try {
			json = mapper.writeValueAsString(this);
		}
		catch(IOException e) {
			e.printStackTrace();
		}

		return json;
	}

	public void fromJson(String json) {
		try {
			Type o = mapper.readValue(json, clazz);
			Mapper beanMapper = new DozerBeanMapper();
			beanMapper.map(o, this);
		}
		catch(Exception e) {
			e.printStackTrace();
		}
	}
}