namespace e8rtp {

    void setup  (int pin, int volume, const char *buffer);
    void loop   (void);
    void start  (void);
    void reset  (void);
    void stop   (void);
    void pause  (void);
    void resume (void);
    int  setVolume (int volume);

} // namespace
