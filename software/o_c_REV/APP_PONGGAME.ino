// (c) 2018, Jason Justian (chysn), Beize Maze
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// CV-controllable Pong game

/* Define the screen boundaries. There's a frame around the screen, so these numbers need to
 * take that into account.
 */
#define BOUNDARY_TOP 11
#define BOUNDARY_BOTTOM 61
#define BOUNDARY_RIGHT 109
#define BOUNDARY_LEFT 2

/* Define player properties. INITIAL_BALL_DELAY is how many screen refresh cycles the ball takes to move. It
 * gets faster as the games goes on. PADDLE_DELAY is how many screen refresh cycles the player must wait before moving
 * again. This is to keep the game interesting at higher levels. PADDLE_WIDTH is the chunkiness of the paddle,
 * in pixels.
 */
#define INITIAL_BALL_DELAY 20
#define PADDLE_DELAY 15
#define PADDLE_WIDTH 3

/* TRIGGER_CYCLE_LENGTH specifies how many ISR cycles a triggered event (like a hit) lasts. */
#define TRIGGER_CYCLE_LENGTH 100

/* This value is used for converting a ball's or paddle's Y position into a pitch value. This number was determined
 * experimentally, since I wasn't sure what the total range for pitch values is.
 */
#define Y_POSITION_COEFF 128

/*
 * When checking the ADC, if I just look for whether the value is positive or negative, then the value can't be
 * zeroed, and the player can't move the paddle with the onboard controls. This is probably because there's a little
 * noise that randomly swirls around 0. So this value simulates a center detent. This is another experimentally-
 * determined value.
 */
#define CENTER_DETENT 640

class PONGGAME {
public:
	/* There are two types of game state properties: Those that should be initialized only once (like high score), and
	 * those that need to be initialized after each game. Init() sets the first kind, and then calls StartNewGame()
	 * to start a new game.
	 */
	void Init() {
		hit_trigger = 0;
		miss_trigger = 0;
		hi_score = 0;

		StartNewGame();
	}

	void StartNewGame() {
		// Game state
		ball_delay = INITIAL_BALL_DELAY;
		ball_countdown = ball_delay;
		paddle_countdown = 0;
		score = 0;

		// Ball properties
		ball_x = 16;
		ball_y = 32;
		dir_x = 1;
		dir_y = 1;

		// Player properties
		paddle_x = 16;
		paddle_y = 1 + BOUNDARY_TOP;
		paddle_h = 16;
	}

	/* ISR() handles game states to do CV input and output */
    void ISR() {
    		/* Handle input states:
    		 * CV 1 is a paddle control. Negative values go up, positive values go down. Note that the value
    		 * is compared to CENTER_DETENT instead of zero. This is to prevent little bits of noise from confusing
    		 * the game.
    		 */
	    int32_t current_value;
	    current_value = OC::ADC::raw_pitch_value(ADC_CHANNEL_1);
	    if (current_value < -CENTER_DETENT) MovePaddleUp();
	    if (current_value > CENTER_DETENT) MovePaddleDown();

    		/* Handle output states:
    		 * Outputs are set as follows. Note that outs A and B are triggers, so it's just a small value transposed
    		 * five octaves up. The trigger time is handled by counting down from TRIGGER_CYCLE_LENGTH. There might be
    		 * a better way to do triggers, and I'll revisit this later.
    		 *
    		 * C and D outs are just scaled values. We have about a 60-step Y value, and I multiply that by Y_POSITION_COEFF
    		 * to get a pitch value to set. I tried to calibrate Y_POSITION_COEFF to get a CV range between 0 and 4-ish volts.
    		 */
    		uint32_t out_A; // Hit reward trigger
    		uint32_t out_B; // Miss punishment trigger
    		uint32_t out_C; // Ball position
    		uint32_t out_D; // Paddle position

		if (hit_trigger) {
			// Strike the hit output
			out_A = 5;
			hit_trigger--;
		} else {
			out_A = 0;
		}

		if (miss_trigger) {
			// Strike the miss output
			out_B = 5;
			miss_trigger--;
		} else {
			out_B = 0;
		}

		out_C = (ball_y - BOUNDARY_TOP) * Y_POSITION_COEFF;
		out_D = ((paddle_y + (paddle_h / 2)) - BOUNDARY_TOP) * Y_POSITION_COEFF; // Mid-paddle here

	    OC::DAC::set_pitch(DAC_CHANNEL_A, 1, out_A);
	    OC::DAC::set_pitch(DAC_CHANNEL_B, 1, out_B);
	    OC::DAC::set_pitch(DAC_CHANNEL_C, out_C, 0);
	    OC::DAC::set_pitch(DAC_CHANNEL_D, out_D, 0);
    }

    int get_score() {return score;}
    int get_hi_score() {return hi_score;}

    void MoveBall() {
    		/* MoveBall() is called with each invocation of PONGGAME_menu(). Moving the ball with each screen refresh would
    		 * make the game unplayable, so movements are delayed with countdowns.
    		 */
    		if (--ball_countdown < 0) {
    			// Move the ball based on the current direction
    			ball_x += dir_x;
			ball_y += dir_y;

			// Check the playfield boundaries. Oh, yes, O_C will crash if you go too far out of bounds.
			if (ball_x > BOUNDARY_RIGHT || ball_x < BOUNDARY_LEFT) {dir_x = -dir_x;}
			if (ball_y > BOUNDARY_BOTTOM || ball_y < BOUNDARY_TOP) {dir_y = -dir_y;}

			// All these conditions are just asking "Did the ball hit the paddle while traveling left?"
			if ((ball_x <= paddle_x + PADDLE_WIDTH) && (ball_x >= paddle_x)
			  && (ball_y <= paddle_y + paddle_h) && (ball_y >= paddle_y) && dir_x < 0) {
				// If so, bounce the ball, increase the score, and set the hit trigger to fire the reward
				// CV trigger at the next ISR() call.
				dir_x = -dir_x;
				score++;
				hit_trigger = TRIGGER_CYCLE_LENGTH;

				// Level up!!
				if (!(score % 5)) LevelUp();
			}

			// Since the point of the game is to defend your left wall, this is when you lose :,(
			if (ball_x < 2) {
				if (score > hi_score) hi_score = score;
				miss_trigger = TRIGGER_CYCLE_LENGTH; // Setting the miss trigger length fires Out B on the next ISR()
				StartNewGame();
			}

			ball_countdown = ball_delay; // Reset delay to start a new movement cycle
    		}

    		/* This is like the above, but for the paddle, and much simpler. The idea of the countdown is to constrain
    		 * the paddle from moving too fast, which is more of an issue when the paddle is under CV control. Surely
    		 * we don't want to make things too easy on your clever AI patches!
    		 */
    		if (--paddle_countdown < 0) {paddle_countdown = 0;}
    }

    /* Performs the LevelUp. The game is designed to get brutal over time. The paddle gets smaller, the
     * ball gets faster, and the paddle gets closer to the wall. Fun times!
     */
    void LevelUp() {
		paddle_h--;
		ball_delay -= 3;
		paddle_x += 4;
		ball_x += 4; // If the ball isn't also moved, it looks weird.

		// Here are some points after which it doesn't get any harder
		if (paddle_h < 4) paddle_h = 4;
		if (ball_delay < 3) ball_delay = 3;
		if (paddle_x > 64) paddle_x = 64;
    }

    /*
     * The ball is just a little 2x2 square, with ball_x and ball_y describing the upper-left corner.
     */
    void DrawBall() {
    		graphics.setPixel(ball_x, ball_y);
    		graphics.setPixel(ball_x + 1, ball_y + 1);
    		graphics.setPixel(ball_x + 1, ball_y);
    		graphics.setPixel(ball_x, ball_y + 1);
    }

    /* The player paddle is a filled rectangle of fixed width and adjustable height. */
    void DrawPlayerPaddle() {
    		graphics.drawRect(paddle_x, paddle_y, PADDLE_WIDTH, paddle_h);
    }

    /* I'm also drawing a paddle for Ornament and Crime, just as visual effect. It's really quite
     * an unfair farce, as it's totally unbeatable.
     */
    void DrawOCPaddle() {
    		int oc_x = BOUNDARY_RIGHT + 2;
    		int oc_y = ball_y - 8;
    		if (oc_y < BOUNDARY_TOP) oc_y = BOUNDARY_TOP;
    		if (oc_y + 16 > BOUNDARY_BOTTOM) oc_y = BOUNDARY_BOTTOM - 16;
    		graphics.drawRect(oc_x, oc_y, PADDLE_WIDTH, 16);
    }

    /* If the paddle countdown is 0, the paddle may move. Whenever the paddle is moved, the downdown begins again
     * to constrain the speed of the paddle. See the bottom of MoveBall() for more deets.
     */
    void MovePaddleUp() {
    		if (paddle_countdown == 0) {
    			--paddle_y;
    			if (paddle_y < BOUNDARY_TOP) paddle_y = BOUNDARY_TOP;
    			paddle_countdown = PADDLE_DELAY;
    		}
    }

    /* Like MovePaddleUp(), only more down */
    void MovePaddleDown() {
    	    if (paddle_countdown == 0) {
    	    		++paddle_y;
    	    		if (paddle_y > (BOUNDARY_BOTTOM - paddle_h)) paddle_y = BOUNDARY_BOTTOM - paddle_h;
    	    		paddle_countdown = PADDLE_DELAY;
    	    }
    }

private:
    int ball_delay;
    int ball_countdown;
    int paddle_countdown;
    int score;
    int hi_score;
    int hit_trigger;
    int miss_trigger;

    int ball_x;
    int ball_y;
    int dir_x;
    int dir_y;

    int paddle_x;
    int paddle_y;
    int paddle_h;
};

PONGGAME pong_instance;

// App stubs
void PONGGAME_init() {
	pong_instance.Init();
}

size_t PONGGAME_storageSize() {
	return 0;
}

size_t PONGGAME_save(void *storage) {
	return 0;
}

size_t PONGGAME_restore(const void *storage) {
	return 0;
}

void PONGGAME_isr() {
	pong_instance.ISR();
}

void PONGGAME_handleAppEvent(OC::AppEvent event) {

}

void PONGGAME_loop() {
}

void PONGGAME_menu() {
	graphics.drawFrame(0, 9, 128, 55);

	pong_instance.MoveBall();
	pong_instance.DrawBall();
    pong_instance.DrawPlayerPaddle();
    pong_instance.DrawOCPaddle();

	graphics.setPrintPos(1,1);
	int score = pong_instance.get_score();
	int hi_score = pong_instance.get_hi_score();
	if (score == 0 && hi_score > 0) {
		graphics.print("O_C Pong High : ");
	    graphics.pretty_print(hi_score);
	} else {
		graphics.print("O_C Pong Score: ");
	    graphics.pretty_print(score);
	}
}

/* I want to see the game all the time, so the screensaver is the same as the main display
 * routine. OLED burn-in is a real thing, so you shouldn't be playing O_C Pong 24/7. And I suppose
 * I could improve this by removing the frame and the score. But really, this is just an
 * instructional app.
 */
void PONGGAME_screensaver() {
	PONGGAME_menu();
}

/* Controlling the game with the buttons is the worst experience ever, so this is really just here
 * to demonstrate how the buttons work.
 */
void PONGGAME_handleButtonEvent(const UI::Event &event) {
	if (UI::EVENT_BUTTON_PRESS == event.type) {
	    switch (event.control) {
	      case OC::CONTROL_BUTTON_UP:
	    	    pong_instance.MovePaddleUp();
	        break;

	      case OC::CONTROL_BUTTON_DOWN:
	    	    pong_instance.MovePaddleDown();
	        break;
	    }
	}
}

/* The UI::Event has a value property, which is positive when the encoder is turned clockwise and
 * negative when it's turned widdershins. I just wanted to say "widdershins."
 */
void PONGGAME_handleEncoderEvent(const UI::Event &event) {
	if (event.value < 0) pong_instance.MovePaddleUp();
	if (event.value > 0) pong_instance.MovePaddleDown();
}
