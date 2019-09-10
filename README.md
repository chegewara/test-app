Test app to show how BLE data can be protected against repeating by unauthorized device. Even if transmission will be intercepted it can't be used because is no longer valid. It's just example with few assumptions:

1. only 1 peer device (client) can be connected at the same time
2. only part of IV key is advertised, which means it's constant and valid during connection time, but without full encryption code and shared private key it is useless
3. if peer device will be authenticated on 1st write message, then no need to change IV key, otherwise device should be disconnected
4. cfb128 encryption has been choosed because of easy usage of IV key (similar procedure, which is IV key, is used in BLE mesh)