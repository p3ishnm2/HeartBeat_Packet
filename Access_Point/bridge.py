# -*- coding: utf-8 -*-
# 心拍数、電圧を取得してBLEでアドバタイズ（ブロードキャスト）
# M5StickCは4秒：アドバタイズ、1秒：Deep Sleep
# ラズパイ側は常時スキャンし、データーを取得したらprintする

from bluepy.btle import DefaultDelegate, Scanner, BTLEException
import sys
import struct
from datetime import datetime
import subprocess

# T_PERIOD = 4


class ScanDelegate(DefaultDelegate):
    def __init__(self):  # コンストラクタ
        DefaultDelegate.__init__(self)
        self.lastseq = None
        self.lasttime = datetime.fromtimestamp(0)
        print("init")

    def handleDiscovery(self, dev, isNewDev, isNewData):
        if isNewDev or isNewData:  # 新しいデバイスまたは新しいデータ
            for (adtype, desc, value) in dev.getScanData():  # データの数だけデータスキャンを繰り返す
                # 'ffff' = テスト用companyID
                # M5stickCからのデータを見つけたら
                if desc == 'Manufacturer' and value[0:4] == 'ffff':
                    __delta = datetime.now() - self.lasttime  # 前回取得時刻からの差分をとる
                    # アドバタイズする1秒(デフォルト：10秒)の間に複数回測定されseqが加算されたものは捨てる（最初に取得された１個のみを使用する）
                    # default seconds = 11
                    if value[4:6] != self.lastseq and __delta.total_seconds() > 5:
                        self.lastseq = value[4:6]  # Seqと時刻を保存 時刻はどこにあるんだ
                        self.lasttime = datetime.now()
                        # (temp, humid, press, volt) = struct.unpack('<hhhh', bytes.fromhex(value[6:]))  # hは2Byte整数（４つ取り出す）
                        (bpm, volt) = struct.unpack_from(
                            '<hh', bytes.fromhex(value[6:]))
                        # print('温度= {0} 度、 湿度= {1} %、 気圧 = {2} hPa、 電圧 = {3} V'.format(temp / 100, humid / 100, press, volt/100))
                        print('心拍数 = {0} bpm, 電圧 = {1} V' .format(
                            bpm, volt/100))
                        # print(bpm)
                        # 遅延の追加
                        if bpm > 80:
                            # ここに直接数値を埋め込めるか？予め文字列に入れるか？
                            loss_netem = (bpm - 80) * 4
                            if loss_netem > 70:
                                loss_netem = 70
                            # delay_netem = bpm - 75  # 遅延量（100bpmで100ms）
                            # jitter_netem = int((bpm - 75) / 2)  # 遅延量のゆらぎ（±）
                            # changeなので指定した値に変更される。累積はされない
                            # cmd_netem = "sudo tc qdisc change dev eth0 root netem delay " + \
                            #     str(delay_netem) + "ms"
                            # + \
                            # str(jitter_netem) + "ms"
                            cmd_netem = "sudo tc qdisc change dev eth0 root netem loss " + \
                                str(loss_netem) + "%" + " delay " + str(loss_netem)
                            # subprocess.runが動いている間はスキャンできないのでは？
                            subprocess.run(
                                cmd_netem, shell=True)
                            print("loss: " + str(loss_netem) + "%")
                        else:
                            # subprocess.run(
                            #     ['sudo tc qdisc change dev eth0 root netem delay 0ms'], shell=True)
                            subprocess.run(
                                ['sudo tc qdisc change dev eth0 root netem loss 0%'], shell=True)


# if __name__ == "__main__":

# 有線'eth0'から出ていくパケットにネットワークエミュレーションを追加
# そもそもこのプログラム自体sudoで動かしているので処理中にsudo passを求められることはないはず
# tc = traffic control
subprocess.run(["sudo tc qdisc add dev eth0 root netem"], shell=True)

scanner = Scanner().withDelegate(ScanDelegate())
try:
    while True:
        # try:
        # スキャンする。デバイスを見つけた後の処理はScanDelegateに任せる
        print("scan")
        scanner.scan(5.0)  # 5秒間スキャンする（この間他の処理は止まる）
# except BTLEException:
    # ex, ms, tb = sys.exc_info()
    # print('BLE exception '+str(type(ms)) +
    #      ' at ' + sys._getframe().f_code.co_name)
    # subprocess.run(["sudo tc qdisc del dev eth0 root"], shell=True)
except KeyboardInterrupt as e:
    # ネットワークエミュレータの削除
    # ここで遅延を元に戻してから終了する
    # subprocess.run(["sudo tc qdisc del dev eth0 root"], shell=True)
    print("Aborted!")
    # pass
finally:
    subprocess.run(["sudo tc qdisc del dev eth0 root"], shell=True)
