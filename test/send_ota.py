import serial
import time
import zlib
from pathlib import Path


# =========================
# 配置
# =========================
PORT = "COM6"
BAUDRATE = 115200

BIN_FILE = "APP.bin"

PACKET_SIZE = 128


# =========================
# CRC16
# 与 STM32 OTA_CRC16 完全一致
# =========================
def crc16_modbus(data: bytes) -> int:
    crc = 0xFFFF

    for byte in data:
        crc ^= byte

        for _ in range(8):
            if crc & 0x0001:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1

    return crc & 0xFFFF


# =========================
# 等待 STM32 返回字符串
# =========================
def wait_response(ser, expected, timeout=3.0):
    start_time = time.time()
    buffer = b""

    while time.time() - start_time < timeout:
        if ser.in_waiting:
            buffer += ser.read(ser.in_waiting)

            try:
                text = buffer.decode("ascii", errors="ignore")
            except Exception:
                text = ""

            if text:
                print(text, end="")

            if expected in text:
                return True

        time.sleep(0.01)

    return False


# =========================
# MAIN
# =========================
def main():

    bin_path = Path(BIN_FILE)

    if not bin_path.exists():
        print(f"找不到文件: {BIN_FILE}")
        return

    firmware = bin_path.read_bytes()

    file_size = len(firmware)
    file_crc32 = zlib.crc32(firmware) & 0xFFFFFFFF

    print("=" * 50)
    print("OTA升级工具")
    print("=" * 50)
    print(f"串口: {PORT}")
    print(f"波特率: {BAUDRATE}")
    print(f"文件大小: {file_size} bytes")
    print(f"CRC32: {file_crc32:08X}")
    print("=" * 50)


    # =========================
    # 打开串口
    # =========================
    try:
        ser = serial.Serial(
            port=PORT,
            baudrate=BAUDRATE,
            timeout=0.1
        )
    except Exception as e:
        print("打开串口失败:")
        print(e)
        return


    # 给STM32一点启动时间
    time.sleep(0.5)

    # 清除之前残留数据
    ser.reset_input_buffer()


    # =========================
    # OTA_START
    #
    # CMD       1
    # FILE_SIZE 4
    # CRC32     4
    # VERSION   16
    # =========================

    version = b"V1.0.0"
    version = version.ljust(16, b"\x00")

    start_packet = (
        bytes([0x01]) +
        file_size.to_bytes(4, byteorder="big") +
        file_crc32.to_bytes(4, byteorder="big") +
        version
    )

    print("\n发送 OTA_START ...")

    ser.write(start_packet)
    ser.flush()

    if not wait_response(ser, "OTA READY", 3):
        print("\nOTA_START失败")
        ser.close()
        return

    print("\nOTA_START成功")


    # =========================
    # OTA_DATA
    # =========================

    total_packets = (
        file_size + PACKET_SIZE - 1
    ) // PACKET_SIZE

    print(f"\n总数据包: {total_packets}")


    for packet_index in range(total_packets):

        seq = packet_index + 1

        offset = packet_index * PACKET_SIZE

        chunk = firmware[
            offset:
            offset + PACKET_SIZE
        ]

        data_len = len(chunk)

        crc16 = crc16_modbus(chunk)


        packet = (
            bytes([0x02]) +
            seq.to_bytes(2, byteorder="big") +
            data_len.to_bytes(2, byteorder="big") +
            chunk +
            crc16.to_bytes(2, byteorder="big")
        )


        print(
            f"\n发送 DATA "
            f"{seq}/{total_packets} "
            f"LEN={data_len} "
            f"CRC16={crc16:04X}"
        )


        ser.write(packet)
        ser.flush()


        if not wait_response(
            ser,
            "OTA DATA OK",
            3
        ):
            print(
                f"\n第 {seq} 包发送失败"
            )

            ser.close()
            return


        print(
            f"第 {seq} 包成功"
        )


        # 稍微给STM32一点时间
        time.sleep(0.01)


    # =========================
    # OTA_END
    #
    # CMD + CRC32
    # =========================

    end_packet = (
        bytes([0x03]) +
        file_crc32.to_bytes(4, byteorder="big")
    )


    print("\n发送 OTA_END ...")

    ser.write(end_packet)
    ser.flush()


    if not wait_response(
        ser,
        "CRC32 OK",
        5
    ):
        print("\nCRC32校验失败")
        ser.close()
        return


    print("\nCRC32校验成功")


    # 等待最终升级完成信息
    if wait_response(
        ser,
        "OTA UPDATE SUCCESS",
        10
    ):
        print("\n==============================")
        print("OTA升级完成")
        print("==============================")
    else:
        print("\n已经收到 CRC32 OK，但没有收到最终升级完成信息")


    ser.close()


if __name__ == "__main__":
    main()
    