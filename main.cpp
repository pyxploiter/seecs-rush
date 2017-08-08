#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <string>
#include <SDL_mixer.h>

//The dimensions of the level
const int LEVEL_WIDTH = 4098;
const int LEVEL_HEIGHT = 700;
const int GRAVITY = 3;

//Screen dimension constants
const int SCREEN_WIDTH = 1000;
const int SCREEN_HEIGHT = 700;

const int startPosX = 200;
const int startPosY = 550;

//The window we'll be rendering tob
SDL_Window* gWindow = NULL;

//The window renderer
SDL_Renderer* gRenderer = NULL;
//The camera area
SDL_Rect camera = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
enum GameState { START, PAUSE, EXIT, WIN, OVER, MENU };
GameState state = MENU;

//Texture wrapper class
class Texture{
public:
	//Initializes variables
	Texture();

	//Deallocates memory
	~Texture();

	//Loads image at specified path
	bool load_image(std::string path);
	//Deallocates texture
	void free();

	//Set color modulation
	void setColor(Uint8 red, Uint8 green, Uint8 blue);

	//Set blending
	void setBlendMode(SDL_BlendMode blending);

	//Set alpha modulation
	void setAlpha(Uint8 alpha);

	//Renders texture at given point
	void render(int x, int y, SDL_Rect* clip = NULL, double angle = 0.0, SDL_Point* center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE);

	//Gets image dimensions
	int getWidth();
	int getHeight();
	//The actual hardware texture
	SDL_Texture* mTexture;
	//Image dimensions
	int mWidth;
	int mHeight;
};

Texture::Texture()
{
	//Initialize
	mTexture = NULL;
	mWidth = 0;
	mHeight = 0;
}

Texture::~Texture()
{
	//Deallocate
	free();
}

bool Texture::load_image(std::string path)
{
	//Get rid of preexisting texture
	free();

	//The final texture
	SDL_Texture* newTexture = NULL;

	//Load image at specified path
	SDL_Surface* loadedSurface = IMG_Load(path.c_str());
	if (loadedSurface == NULL)
	{
		printf("Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
	}
	else
	{
		//Color key image
		SDL_SetColorKey(loadedSurface, SDL_TRUE, SDL_MapRGB(loadedSurface->format, 0, 0xFF, 0xFF));

		//Create texture from surface pixels
		newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
		if (newTexture == NULL)
		{
			printf("Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError());
		}
		else
		{
			//Get image dimensions
			mWidth = loadedSurface->w;
			mHeight = loadedSurface->h;
		}

		//Get rid of old loaded surface
		SDL_FreeSurface(loadedSurface);
	}

	//Return success
	mTexture = newTexture;
	return mTexture != NULL;
}

void Texture::free()
{
	//Free texture if it exists
	if (mTexture != NULL)
	{
		SDL_DestroyTexture(mTexture);
		mTexture = NULL;
		mWidth = 0;
		mHeight = 0;
	}
}

void Texture::setColor(Uint8 red, Uint8 green, Uint8 blue)
{
	//Modulate texture rgb
	SDL_SetTextureColorMod(mTexture, red, green, blue);
}

void Texture::setBlendMode(SDL_BlendMode blending)
{
	//Set blending function
	SDL_SetTextureBlendMode(mTexture, blending);
}

void Texture::setAlpha(Uint8 alpha)
{
	//Modulate texture alpha
	SDL_SetTextureAlphaMod(mTexture, alpha);
}

void Texture::render(int x, int y, SDL_Rect* clip, double angle, SDL_Point* center, SDL_RendererFlip flip){
	//Set rendering space and render to screen
	SDL_Rect renderQuad = { x, y, mWidth, mHeight };

	//Set clip rendering dimensions
	if (clip != NULL)
	{
		renderQuad.w = clip->w;
		renderQuad.h = clip->h;
	}

	//Render to screen
	SDL_RenderCopyEx(gRenderer, mTexture, clip, &renderQuad, angle, center, flip);
}

int Texture::getWidth()
{
	return mWidth;
}

int Texture::getHeight()
{
	return mHeight;
}

class Character{
protected:
	//Direction of character
	char direction;
public:
	//The X and Y offsets character
	int mPosX, mPosY;
	void draw(SDL_Rect* src, int x, int y);
	SDL_Rect current_sprite;
	Texture character_texture;
	bool onMove;
	bool death;
	//Moves the Player
	void move_left();
	void move_right();
	void isRunning();
	void isDead();
	//Position accessors
	int getPosX();
	int getPosY();
	bool collideScreen();
	float speed;
};

void Character::draw(SDL_Rect* src, int x, int y){
	//Set rendering space and render to screen
	SDL_Rect renderQuad = { mPosX - x, mPosY - y, character_texture.mWidth, character_texture.mHeight };
	//Set clip rendering dimensions
	if (src != NULL){
		renderQuad.w = src->w;
		renderQuad.h = src->h;
	}
	SDL_RenderCopy(gRenderer, character_texture.mTexture, src, &renderQuad);
}

void Character::isDead(){
	death = 1;
}
int Character::getPosX()
{
	return mPosX;
}

int Character::getPosY()
{
	return mPosY;
}

void Character::move_left()
{
	direction = 'l';
	isRunning();
}

void Character::move_right(){
	direction = 'r';
	isRunning();
}

void Character::isRunning(){
	onMove = 1;
}

class Enemy : public Character{
private:
	Texture dog;
	Texture mummy;
	SDL_Rect dog_left[6];
	SDL_Rect dog_right[6];
	SDL_Rect mummy_move[5];
	SDL_Rect mummy_death[5];
	int frame;
	int frameRate;
	
public:
	Enemy();
	Enemy(int, int,char);
	void load_sprite();
	void draw_enemy();
	void move();
	bool collision(SDL_Rect player);
	char enemy_type;
};

Enemy::Enemy(){}

Enemy::Enemy(int x, int y,char e_type){
	death = 0;
	frame = 0;
	frameRate = 10;
	mPosX = x;
	mPosY = y;
	enemy_type = e_type;
	speed = 2;
}

bool Enemy::collision(SDL_Rect player){
	//The sides of the rectangles
	int leftA, leftB;
	int rightA, rightB;
	int topA, topB;
	int bottomA, bottomB;

	//Calculate the sides of rect A
	leftA = player.x;
	rightA = player.x + player.w-65;
	topA = player.y;
	bottomA = player.y + player.h-16;
	
	//Calculate the sides of rect B
	leftB = mPosX;
	rightB = mPosX+47;
	topB = mPosY;
	bottomB = mPosY+62;

	//If any of the sides from A are outside of B
	if (bottomA <= topB )
	{
		return false;
	}

	if (topA >= bottomB)
	{
		return false;
	}

	if (rightA <= leftB)
	{
 		return false;
	}

	if (leftA >= rightB)
	{
		return false;
	}
	//If none of the sides from A are outside B
	return true;
}

void Enemy::load_sprite(){
	if (enemy_type == 'd'){
		dog.load_image("assets/dog.png");
		int x = 0, y = 70, w = 76, h = 45;
		for (int i = 0; i < 6; i++){
			y = 70;
			dog_left[i].x = x; dog_left[i].y = y;
			dog_left[i].w = w; dog_left[i].h = h;
			y += 117;
			dog_right[i].x = x; dog_right[i].y = y;
			dog_right[i].w = w; dog_right[i].h = h;
			x += 118;
			if (x > 590)
				x = 0;
		}
	}
	else if (enemy_type == 'm'){
		mummy.load_image("assets/mummy.png");
		int x1 = 5, y1 = 50, w1 = 47, h1 = 62;
		for (int i = 0; i < 5; i++){
			y1 = 50;
			mummy_move[i].x = x1; mummy_move[i].y = y1;
			mummy_move[i].w = w1; mummy_move[i].h = h1;
			y1 += 231;
			mummy_death[i].x = x1; mummy_death[i].y = y1;
			mummy_death[i].w = w1; mummy_death[i].h = h1;
			x1 += 119;
			if (x1 > 340)
				x1 = 0;
		}
	}
}

void Enemy::draw_enemy(){
	if (death == 0){
		if (enemy_type == 'd'){
			current_sprite = dog_left[frame / frameRate];
			frame++;
			if (frame > 59)
				frame = 0;
			dog.render(mPosX, mPosY, &current_sprite, 0, 0, SDL_FLIP_NONE);
		}
		else if (enemy_type == 'm'){
			current_sprite = mummy_move[frame / frameRate];
			frame++;
			if (frame > 49)
				frame = 0;
			//draw(&current_sprite, camera.x, camera.y);
			mummy.render(mPosX, mPosY, &current_sprite, 0, 0, SDL_FLIP_NONE);
		}
	}
	/*draw(&current_sprite, camera.x, camera.y);*/
}

void Enemy::move(){
	mPosX -= speed;
}
class Player: public Character{
public:
	//The dimensions of the Player
	static const int Player_WIDTH = 20;
	static const int Player_HEIGHT = 20;
	SDL_Rect collisionTest;
	SDL_Rect shoot_collision;
	//Initializes the variables
	Player();

	void isJumping();
	void isAttacking();
	void load_sprites();
	//Takes key presses and adjusts the Player's velocity
	void handleEvent(SDL_Event& e);
	void draw_image();
	void playerPosition();
	void enemy_collision();
	bool collideScreen_left();
	bool collideScreen_right();
	//Shows the Player on the screen relative to the camera
	void render(int camX, int camY);
	void shoot(SDL_Rect* src, int x, int y);
	bool onGround;
	bool onJump;
	int frame;
	int frame_rate;
	bool onAttack;
	bool onPower;
	int Jump_Height;
	bool attacked;
	static int blast;
	int blastX;
	int blastY;
private:
	SDL_Rect idle_left[4];
	SDL_Rect idle_right[4];
	SDL_Rect run_left[4];
	SDL_Rect run_right[4];
	SDL_Rect jump_left[4];
	SDL_Rect jump_right[4];
	SDL_Rect power_left[4];
	SDL_Rect power_right[4];
	SDL_Rect attack_left[2];
	SDL_Rect attack_right[2];
	SDL_Rect hurt_left[3];
	SDL_Rect hurt_right[3];
	//SDL_Rect spawn_sprite;
};

int Player::blast = 55;
Player::Player()
{
	speed = 3;
	frame = 0;
	frame_rate = 10;
	//Initialize the offsets
	mPosX = startPosX;
	mPosY = startPosY;
	collisionTest.x = mPosX;
	collisionTest.y = mPosY;
	collisionTest.w = 115;
	collisionTest.h = 120;
	onAttack = 0;
	onPower = 0;
	direction = 'r';
	onGround = 1;
	onMove = 0;
	onJump = 0;
	death = 0;
	attacked = 0;
	/*spawn_sprite = { NULL, NULL, NULL, NULL };*/
}

void Player::isAttacking(){
	onAttack = 1;
}

void Player::isJumping(){
	if (onGround != 0){
		onJump = 1;
		Jump_Height = 6;
		onGround = 0;
	}
}

bool Player::collideScreen_left(){
	if (mPosX < 1)
		return true;
	return false;
}

bool Player::collideScreen_right(){
	if (mPosX >= (LEVEL_WIDTH-150)){
		return true;
		state = WIN;
	}
	return false;
}

void Player::enemy_collision(){
	isDead();
}

void Player::shoot(SDL_Rect* src, int x, int y){
	SDL_Rect renderQuad;
	//Set rendering space and render to screen
	if (direction == 'r'){
		renderQuad = { mPosX - x + blast, startPosY - y + 45, character_texture.mWidth, character_texture.mHeight };
		blast += 4;
		blastX = mPosX - x + blast;
		blastY = mPosY - y + 45;
	}
	else if (direction == 'l'){
		renderQuad = { mPosX - x - blast, startPosY - y + 45, character_texture.mWidth, character_texture.mHeight };
		blast += 4;
		blastX = mPosX - x + blast;
		blastY = mPosY - y + 45;
	}
	if (blast > 600){
		blast = 0;
		attacked = 0;
	}
	shoot_collision.x = blastX;
	shoot_collision.y = blastY;
	//Set clip rendering dimensions
	if (src != NULL){
		renderQuad.w = src->w;
		renderQuad.h = src->h;
	}
	SDL_RenderCopy(gRenderer, character_texture.mTexture, src, &renderQuad);
}

void Player::playerPosition(){
	static int height = 0;
	if (onJump != 0){
		height++;
		if (height == 4 || height == 6 || height == 10 || height == 18 || height == 45 || height == 50){
			Jump_Height--;
		}
		if (Jump_Height <= 0){
			onJump = 0;
			Jump_Height = 0;
			height = 0;
			onGround = 0;
		}
		
		mPosY -= Jump_Height;
		collisionTest.y = mPosY;
	}
	if (onGround != 1 && onJump != 1 ){
		if (Jump_Height < GRAVITY){
			Jump_Height += 1;
		}
		else{
			Jump_Height = GRAVITY;
		}
		mPosY += Jump_Height;
	}
	if (mPosY >= 550){
		onGround = 1;
	}
	if (onMove == 1 && direction == 'r')
		mPosX += speed;
	if (onMove == 1 && direction == 'l')
		mPosX -= speed;
	if (mPosX < 1){
		speed = 0;
		mPosX++;
	}
	else if (onPower != 1)
		speed = 3;
	else if (onPower == 1){
		speed = 6;
	}
	collisionTest.y = mPosY;

}

void Player::handleEvent(SDL_Event& e)
{
	onMove = 0;
	//If a key was pressed
	if (e.type == SDL_KEYDOWN)
	{
		//Adjust the velocity
		if (e.key.keysym.sym == SDLK_UP && onJump != 1)
			isJumping();
		if (e.key.keysym.sym == SDLK_LEFT)
			move_left();
		if (e.key.keysym.sym == SDLK_RIGHT)
			move_right();
		if (e.key.keysym.sym == SDLK_SPACE){
			if (onAttack == 0 && attacked == 0 && onJump != 1)
				isAttacking();
		}
		if (e.key.keysym.sym == SDLK_LCTRL){
			onPower = 1;
			speed = 6;
		}
	}
}

void Player::draw_image(){
	static int sprite = 0;
	if (death == 1){
		if (direction == 'r')
			current_sprite = hurt_right[sprite / (frame_rate+10)];
		else if (direction == 'l')
			current_sprite = hurt_left[sprite / (frame_rate + 10)];
	}
	else{
		if (onMove == 1){
			if (direction == 'r')
				current_sprite = run_right[sprite / frame_rate];
			else if (direction == 'l')
				current_sprite = run_left[sprite / frame_rate];
		}
		else if (onMove == 0){
			if (direction == 'r')
				current_sprite = idle_right[sprite / frame_rate];
			else if (direction == 'l')
				current_sprite = idle_left[sprite / frame_rate];
		}
		if (onJump == 1 || onGround == 0){
			if (direction == 'r')
				current_sprite = jump_right[sprite / frame_rate];
			else if (direction == 'l')
				current_sprite = jump_left[sprite / frame_rate];
			if (sprite > 25){
				sprite = 0;
			}
		}
		if (onMove == 1 && onPower == 1){
			if (direction == 'r')
				current_sprite = power_right[sprite /frame_rate];
			if (direction == 'l')
				current_sprite = power_left[sprite /frame_rate];
			if (sprite > 28)
				sprite = 0;
		}
		if (onAttack == 1 && attacked != 1 && onJump != 1 && onGround != 0){
			if (direction == 'r')
				current_sprite = attack_right[sprite /(20+frame_rate)];
			if (direction == 'l')
				current_sprite = attack_left[sprite /(20+ frame_rate)];
			if (sprite > 25)
				sprite = 0;
			if (sprite == 25)
				attacked = 1;
		}
	}
	sprite++;
	if (sprite > 39)
		sprite = 0;

	if (sprite == 0)
		onAttack = 0;
	draw(&current_sprite, camera.x, camera.y);
	if (attacked == 1){
		if (direction == 'r'){
			shoot(&attack_right[1], camera.x, camera.y);
		}
		else if (direction == 'l'){
			shoot(&attack_left[1], camera.x, camera.y);
		}
	}
}

void Player::load_sprites(){
	int Frame = 4;
	character_texture.load_image("assets/player.png");
	int x3 = 0, y3 = 0, w3 = 115, h3 = 120;
	int yDifference = 117;
	for (int i = 0; i < Frame; i++){
		y3 = 0;
		idle_right[i].x = x3;				idle_right[i].y = y3;
		idle_right[i].w = w3;				idle_right[i].h = h3;
		y3 += yDifference;
		idle_left[Frame - i - 1].x = x3;		idle_left[Frame - i - 1].y = y3;
		idle_left[Frame - i - 1].w = w3;		idle_left[Frame - i - 1].h = h3;
		y3 += yDifference;
		run_right[i].x = x3;					run_right[i].y = y3;
		run_right[i].w = w3;					run_right[i].h = h3;
		y3 += yDifference;
		run_left[Frame - i - 1].x = x3;		run_left[Frame - i - 1].y = y3;
		run_left[Frame - i - 1].w = w3;		run_left[Frame - i - 1].h = h3;
		y3 += yDifference+2;
		jump_right[i].x = x3;				jump_right[i].y = y3;
		jump_right[i].w = w3;				jump_right[i].h = h3;
		y3 += yDifference+3;
		jump_left[Frame - i - 1].x = x3;		jump_left[Frame - i - 1].y = y3;
		jump_left[Frame - i - 1].w = w3;		jump_left[Frame - i - 1].h = h3;
		y3 += yDifference+5;
		power_right[i].x = x3+120;				power_right[i].y = y3;
		power_right[i].w = w3;				power_right[i].h = h3;
		y3 += yDifference+5;
		power_left[Frame - i - 2].x = x3+122;	power_left[Frame - i - 2].y = y3;
		power_left[Frame - i - 2].w = w3;	power_left[Frame - i - 2].h = h3;
		x3 += 120;
		if (x3 > 500)
			x3 = 0;
	}
	int x = 0, y = 1192, h = 123, w = 60;
	for (int i = 0; i < 3; i++){
		y = 1192;
		hurt_right[i].x = x;				hurt_right[i].y = y;
		hurt_right[i].w = w;				hurt_right[i].h = h;
		y += yDifference+7;
		hurt_left[Frame - i - 2].x = x;		hurt_left[Frame - i - 2].y = y;
		hurt_left[Frame - i - 2].w = w;		hurt_left[Frame - i - 2].h = h;
		x += 122;
		if (x>366)
			x = 0;
	}
	attack_right[0].x = 10; attack_right[0].y = 965;
	attack_right[0].w = 112; attack_right[0].h = 100;

	attack_left[0].x = 7; attack_left[0].y = 1082;
	attack_left[0].w = 112; attack_left[0].h = 100;
	
	/*attack_right[1].x = 148; attack_right[1].y = 964;
	attack_right[1].w = 56; attack_right[1].h = 100;
	
	attack_left[1].x = 144; attack_left[1].y = 1082;
	attack_left[1].w = 56; attack_left[1].h = 100;*/
	
	attack_right[1].x = 227; attack_right[1].y = 1009;
	attack_right[1].w = 74; attack_right[1].h = 59;

	attack_left[1].x = 228; attack_left[1].y = 1115;
	attack_left[1].w = 74; attack_left[1].h = 59;

	shoot_collision.h = 59;	shoot_collision.w = 74;
}

class GamePlay{
private:
	Player Player;
	Enemy *enemies[10];
	//Scene textures
	Texture gPlayerTexture;
	Texture background;
	Texture menu[7];
	Texture win;
	Texture over;
	Mix_Music *Music;
public:
	//Starts up SDL and creates window
	bool init();

	//Loads media
	bool loadMedia();

	//Frees media and shuts down SDL
	void close();
	void start();
	bool checkCollision();
	bool checkButton(SDL_Event e, int x1, int x2, int y1, int y2);
	void camera_control();
	void Menu();
};

bool GamePlay::init()
{
	Music = NULL;
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		//Set texture filtering to linear
		if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
		{
			printf("Warning: Linear texture filtering not enabled!");
		}

		//Create window
		gWindow = SDL_CreateWindow("SEECS RUSH", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (gWindow == NULL)
		{
			printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
			success = false;
		}
		else
		{
			//Create vsynced renderer for window
			gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			if (gRenderer == NULL)
			{
				printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
				success = false;
			}
			else
			{
				//Initialize renderer color
				SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

				//Initialize PNG loading
				int imgFlags = IMG_INIT_PNG;
				if (!(IMG_Init(imgFlags) & imgFlags))
				{
					printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
					success = false;
				}
			}
			if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
			{
				printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
				success = false;
			}
		}
	}
	enemies[0] = new Enemy(Player.collisionTest.x + 800, Player.collisionTest.y + 60, 'd');
	enemies[1] = new Enemy(Player.collisionTest.x + 1200, Player.collisionTest.y + 60, 'd');
	enemies[2] = new Enemy(Player.collisionTest.x + 1500, Player.collisionTest.y + 50, 'm');
	enemies[3] = new Enemy(Player.collisionTest.x + 1900, Player.collisionTest.y + 60, 'd');
	enemies[4] = new Enemy(Player.collisionTest.x + 2300, Player.collisionTest.y + 50, 'm');
	enemies[5] = new Enemy(Player.collisionTest.x + 2600, Player.collisionTest.y + 60, 'd');
	enemies[6] = new Enemy(Player.collisionTest.x + 3000, Player.collisionTest.y + 50, 'm');
	enemies[7] = new Enemy(Player.collisionTest.x + 2800, Player.collisionTest.y + 50, 'm');
	enemies[8] = new Enemy(Player.collisionTest.x + 3200, Player.collisionTest.y + 60, 'd');
	enemies[9] = new Enemy(Player.collisionTest.x + 3400, Player.collisionTest.y + 50, 'm');
	enemies[10] = new Enemy(Player.collisionTest.x + 3600, Player.collisionTest.y + 50, 'm');
	return success;
}
bool GamePlay::loadMedia()
{
	//Loading success flag
	bool success = true;

	Music = Mix_LoadMUS("assets/music.mp3");
	//Load Player texture
	/* if (!gPlayerTexture.load_image("assets/player1.png"))
	{
		printf("Failed to load Player texture!\n");
		success = false;
	}
	*/
	//Load background texture
	if (!background.load_image("assets/background1.png"))
	{
		printf("Failed to load background texture!\n");
		success = false;
	}
	win.load_image("assets/win.png");
	over.load_image("assets/over.png");
	menu[0].load_image("assets/menu.png");
	menu[1].load_image("assets/menu1.png");
	menu[2].load_image("assets/menu2.png");
	menu[3].load_image("assets/menu3.png");
	menu[4].load_image("assets/menu4.png");
	menu[5].load_image("assets/highscore.png");
	menu[6].load_image("assets/instructions.png");
	Player.load_sprites();
	for (int i = 0; i < 6; i++)
		enemies[i]->load_sprite();
	return success;
}

void GamePlay::Menu(){
	SDL_Event Event;
	int flag = 0;
	while (state == MENU || state == PAUSE)
	{
		SDL_PollEvent(&Event);
		//Set mouse over sprite
		if (Event.type == SDL_MOUSEBUTTONUP && checkButton(Event, 40, 274, 165, 224) != true){
			SDL_RenderCopy(gRenderer, menu[1].mTexture, NULL, NULL);
			state = START;
		}
		else if (Event.type == SDL_MOUSEBUTTONUP && checkButton(Event, 40, 274, 272, 333) != true){
			if (flag == 0){
				SDL_RenderCopy(gRenderer, menu[2].mTexture, NULL, NULL);
				SDL_RenderPresent(gRenderer);
				SDL_Delay(150);
				flag = 1;
			}
			SDL_RenderCopy(gRenderer, menu[6].mTexture, NULL, NULL);
		}
		else if (Event.type == SDL_MOUSEBUTTONUP && checkButton(Event, 40, 274, 384, 449) != true){
			if (flag == 0){
				SDL_RenderCopy(gRenderer, menu[3].mTexture, NULL, NULL);
				SDL_RenderPresent(gRenderer);
				SDL_Delay(150);
				flag = 1;
			}
			SDL_RenderCopy(gRenderer, menu[5].mTexture, NULL, NULL);
		}
		else if (Event.type == SDL_MOUSEBUTTONUP && checkButton(Event, 40, 274, 486, 556) != true){
			SDL_RenderCopy(gRenderer, menu[4].mTexture, NULL, NULL);
			state = EXIT;
		}
		else{
			SDL_RenderCopy(gRenderer, menu[0].mTexture, NULL, NULL);
			flag = 0;
		}
		SDL_RenderPresent(gRenderer);
		SDL_Delay(100);
	}
}

bool GamePlay::checkCollision(){
	for (int i = 0; i < 11; i++){
		if (enemies[i]->collision(Player.collisionTest) && enemies[i]->death == 0){
			Player.enemy_collision();
			return true;
		}
		if (enemies[i]->collision(Player.shoot_collision) && enemies[i]->death == 0 && Player.attacked == 1){
			enemies[i]->isDead();
			Player.attacked = 0;
			Player.blast = 0;
		}
	}
	return false;
}

bool GamePlay::checkButton(SDL_Event e, int x1, int x2, int y1, int y2){
	//Check if mouse is in button
	bool inside = false;
	if (state == MENU){
		//If mouse event happened
		if (e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP)
		{
			//Get mouse position
			int x, y;
			SDL_GetMouseState(&x, &y);
			/*printf("%d-%d\n", x, y);*/
			//Mouse is left of the button
			if (x < x1)
			{
				inside = true;
			}
			//Mouse is right of the button
			else if (x > x2)
			{
				inside = true;
			}
			//Mouse above the button
			else if (y < y1)
			{
				inside = true;
			}
			//Mouse below the button
			else if (y > y2)
			{
				inside = true;
			}
		}
		return inside;
	}
}

void GamePlay::camera_control(){
	//Center the camera over the Player
	camera.x = (Player.getPosX() + Player::Player_WIDTH / 2) - SCREEN_WIDTH / 2 + 300;
	camera.y = (Player.getPosY() + Player::Player_HEIGHT / 2) - SCREEN_HEIGHT / 2;
	//Keep the camera in bounds
	if (camera.x < 0)
	{
		camera.x = 0;
	}
	if (camera.y < 0)
	{
		camera.y = 0;
	}
	if (camera.x > LEVEL_WIDTH - camera.w)
	{
		camera.x = LEVEL_WIDTH - camera.w;
	}
	if (camera.y > LEVEL_HEIGHT - camera.h)
	{
		camera.y = LEVEL_HEIGHT - camera.h;
	}
}

void GamePlay::close()
{
	//Free loaded images
	gPlayerTexture.free();
	background.free();
	for (int i = 0; i < 7; i++){
		menu[i].free();
	}
	win.free();
	over.free();
	//Destroy window	
	SDL_DestroyRenderer(gRenderer);
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;
	gRenderer = NULL;

	//Quit SDL subsystems
	IMG_Quit();
	SDL_Quit();
}

void GamePlay::start(){
	//Start up SDL and create window
	if (!init())
	{
		printf("Failed to initialize!\n");
	}
	else
	{
		//Load media
		if (!loadMedia())
		{
			printf("Failed to load media!\n");
		}
		else
		{
			//Event handler
			SDL_Event e;
			while (state != EXIT && state != OVER && state != WIN){
				SDL_PollEvent(&e);
				if (e.type == SDL_QUIT){
					break;
				}
				Menu();
				if (Player.collideScreen_right() == 1){
					state = WIN;
					break;
				}

				if (checkCollision() == true || Player.death == 1){
					static int clock = 0;
					clock++;
					if (clock > 45){ //delay to show hurt animation
						state = OVER;
					}
				}
				if (Mix_PlayingMusic() == 0)
				{
					//Play the music
					Mix_PlayMusic(Music, -1);
				}
				camera_control();
				//Clear screen
				SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
				SDL_RenderClear(gRenderer);
				//Render background
				background.render(0, 0, &camera);
				Player.playerPosition();
				for (int i = 0; i < 11; i++){
					enemies[i]->draw_enemy();
					enemies[i]->move();
				}
				//Handle input for the Player
				Player.handleEvent(e);
				Player.draw_image();
				//Player.render(camera.x, camera.y);
				//Update screen
				SDL_RenderPresent(gRenderer);
			}

			if (state == OVER){
				SDL_RenderCopy(gRenderer, over.mTexture, NULL, NULL);
				SDL_RenderPresent(gRenderer);
				SDL_Delay(2500);
			}
			if (state == WIN){
				SDL_RenderCopy(gRenderer, win.mTexture, NULL, NULL);
				SDL_RenderPresent(gRenderer);
				SDL_Delay(5000);
			}
		}
	}
	//Free resources and close SDL
	close();

}

int main(int argc, char* args[])
{
	GamePlay game;
	game.start();
	return 0;
}