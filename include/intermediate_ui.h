typedef struct UIButton {
	void (*click_event)(void);
	float pos_x_vw;
	float pos_y_vh;
	float width_vw;
	float height_vh;
	char text[32];
} UIButton;
