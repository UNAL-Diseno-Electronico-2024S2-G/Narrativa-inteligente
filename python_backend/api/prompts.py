from langchain_core.prompts import ChatPromptTemplate, MessagesPlaceholder
from langchain_core.messages import HumanMessage

DEVICE_NAME = 'ALVA'

def format_system_msg(user_data):
    """Generates a system message based on user data to personalize responses."""
    system_msg = f"""You are an assistant called {DEVICE_NAME}, designed to help a blind person 
    experience their surroundings through image descriptions. 

    You have access to the following **user details** to personalize responses.  
    **Do NOT explicitly repeat or list these details in your response.** Instead, use them naturally 
    to make the conversation more engaging.  

    - User's name: {user_data.get("user_name", "N/A")}  
    - Age: {user_data.get("user_age", "N/A")}  
    - Total blindness? {user_data.get("total_blind", "N/A")}  
    - Partial vision details: {user_data.get("blind_details", "N/A")}  
    - Preferred language: {user_data.get("user_lang", "N/A")}  
    - Uses Braille? {user_data.get("braille", "N/A")}  
    - Screen reader or communication tools: {user_data.get("devices_details", "N/A")}  
    - Mobility: {user_data.get("mobility", "N/A")}  
    - Independence level: {user_data.get("independence_level", "N/A")}  
    - Use case for {DEVICE_NAME}: {user_data.get("use_case", "N/A")}  
    - Preferred reading speed: {user_data.get("reading_speed", "N/A")}  
    - Voice preference: {user_data.get("voice_preference", "N/A")}  
    - Preferred tone: {user_data.get("tone_preference", "N/A")}  
    - Detail level: {user_data.get("detail_level", "N/A")}  
    - Wants extra information (colors, textures, etc.): {user_data.get("extra_info", "N/A")}  
    - Current devices: {user_data.get("current_devices", "N/A")}  

    **Your goal:**  
    - **Do NOT repeat this data explicitly.**  
    - Make responses engaging and tailored to the user.  
    - Address the user by name when possible.  
    - Adjust descriptions to their **level of detail, voice, and preferred information**.  
    """
    return system_msg


def format_human_msg(user_data, image_data):
    """Creates a user message with an image prompt for the model."""
    user_name = user_data.get("user_name", "there")  # Usa "there" si no hay nombre
    detail_level = user_data.get("detail_level", "detailed")  # Preferencia del usuario

    message = HumanMessage(
        content=[
            {
                "type": "text",
                "text": (
                    f"Hi {user_name}, please describe this image with **maximum detail**. "
                    "Make sure to include all key elements, colors, shapes, textures, lighting conditions, and any recognizable objects or people. "
                    "If the user prefers additional information, describe possible emotions, context, or interactions in the scene. "
                    f"Adjust the tone to be {user_data.get('tone_preference', 'natural and friendly')} and the level of detail should be {detail_level}."
                )
            },
            {
                "type": "image_url",
                "image_url": {"url": f"data:image/jpeg;base64,{image_data}"},
            },
        ]
    )
    return message


def format_prompts(user_data):
    """Genera el prompt con los datos del usuario y el mensaje del sistema."""
    system_msg = format_system_msg(user_data)

    prompt_template = ChatPromptTemplate.from_messages([
        ("system", system_msg),
        MessagesPlaceholder(variable_name="msgs")  # Para futuros mensajes de usuario o IA
    ])

    return prompt_template
