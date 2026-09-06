main() {
    level waittill("connected", player);
    player thread bo1zt\core\startup::init_();
    player thread bo1zt\core\api::init_();
    player thread bo1zt\core\workers::init_();
    player thread bo1zt\core\listeners::init_();
}
