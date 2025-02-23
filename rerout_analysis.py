import os
import re
import glob

def convertRtt(value):
    if value.endswith("us"):
        return int(value[:-2])
    elif value.endswith("ms"):
        return int(value[:-2]) * 1000
    elif value.endswith("s"):
        return int(value[:-1]) * 1000 * 1000
    else:
        return int(value)

def extractReroutedOrders():
    reroutingPattern = re.compile(r"Rerouting (\d+)")
    orders = set()

    serverLogFiles = glob.glob("server_log.txt") + glob.glob("server_log.[0-9]*.txt")
    
    for logFile in serverLogFiles:
        if os.path.exists(logFile):
            print(f"Extracting rerouted orders from {logFile}")
            with open(logFile, 'r') as file:
                for line in file:
                    match = reroutingPattern.search(line)
                    if match:
                        # print(f"Found rerouted order {match.group(1)}")
                        orders.add(match.group(1))
    return orders


def analyzeReroutedRtt(orders):
    traderLogFiles = glob.glob("trader_log.txt") + glob.glob("trader_log.[0-9]*.txt")
    rtts = []

    for logFile in traderLogFiles:
        if os.path.exists(logFile):
            print(f"Analyzing trader log file {logFile}")
            with open(logFile, 'r') as file:
                for line in file:
                    for order in orders:
                        if str(order) in line:
                            words = line.strip().split()
                            if len(words) > 1:
                                rtts.append(convertRtt(words[-1]))
                                # print(f"Rerouted order {order}, Rtt: {words[-1]}")
                            break

    return rtts

def convertToScale(rtt):
    if rtt < 1000:
        return f"{rtt}us"
    elif rtt >= 1000 and rtt < 1000000:
        return f"{rtt // 1000}ms"
    else:
        return f"{rtt // 1000000}s"

def getRttStats(rtts):
    minRtt = min(rtts)
    maxRtt = max(rtts)
    avgRtt = sum(rtts) / len(rtts)

    return minRtt, maxRtt, avgRtt

def main():
    os.chdir('build')

    orders = extractReroutedOrders()
    rtts = analyzeReroutedRtt(orders)
    minRtt, maxRtt, avgRtt = getRttStats(rtts)
    print(f"Rerouted orders {len(rtts)} Rtt min:{convertToScale(minRtt)} max:{convertToScale(maxRtt)} avg:{convertToScale(avgRtt)}")

if __name__ == "__main__":
    main()
