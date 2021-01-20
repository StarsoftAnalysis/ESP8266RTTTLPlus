namespace e8rtp {

    void setup  (int pin, int volume, const char *buffer);
    void loop   (void);
    void start  (void);
    void reset  (void);
    void stop   (void);
    void pause  (void);
    void resume (void);
    int  setVolume (int volume);

    enum stateEnum { Unready, Ready, Playing, Paused };
    stateEnum state (void);

} // namespace
