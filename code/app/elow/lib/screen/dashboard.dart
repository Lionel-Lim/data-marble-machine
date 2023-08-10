import 'dart:async';

import 'package:cloud_firestore/cloud_firestore.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:intl/intl.dart';
import 'package:syncfusion_flutter_charts/charts.dart';
import 'package:syncfusion_flutter_gauges/gauges.dart';

enum ReadingStatus { on, off, error }

class Dashboard extends StatefulWidget {
  final UserCredential credential;
  const Dashboard({super.key, required this.credential});

  @override
  State<Dashboard> createState() => _DashboardState();
}

class _chartData {
  _chartData(this.x, this.y);
  final DateTime x;
  final double y;
}

const List<Widget> fruits = <Widget>[
  Text('Live'),
  Text('History'),
];

class _DashboardState extends State<Dashboard>
    with SingleTickerProviderStateMixin {
  late final AnimationController _controller;
  late Stream<QuerySnapshot> _docStream;
  late String _uid;
  late int maxLiveUse;
  double liveUse = 0;
  String status = "Off";
  late StreamSubscription stream;
  List<_chartData> liveHistory = [];
  List<_chartData> dailyHistory = [];
  bool isRefreshing = true;
  final List<bool> _selectedMenu = <bool>[true, false];
  int _selectedMenuIndex = 0;

  FirebaseAuth auth = FirebaseAuth.instance;

  int getMaxDaily() {
    int maxPower = 100;
    FirebaseFirestore.instance
        .collection('device/$_uid/sensors/overall/live/')
        .orderBy("power", descending: true)
        .limit(1)
        .get()
        .then(
      (snapshot) {
        final docs = snapshot.docs;
        if (docs.isNotEmpty) {
          // Assuming you want to listen to the first document changes, you can implement different logic if needed
          final data = docs.first.data();
          debugPrint("power is $data");
          maxPower = data['power'] as int;
        } else {
          debugPrint("No data");
        }
      },
    );
    return maxPower;
  }

  void setStatus(ReadingStatus readingStatus) {
    setState(() {
      switch (readingStatus) {
        case ReadingStatus.on:
          status = "Live";
          break;
        case ReadingStatus.off:
          status = "Off";
          break;
        case ReadingStatus.error:
          status = "Error";
          break;
      }
    });
  }

  Widget graphTest(List<_chartData> data) {
    // debugPrint("Graph data is $data");
    return SfCartesianChart(
      primaryXAxis: DateTimeAxis(
        // intervalType: DateTimeIntervalType.days,
        // interval: 1,
        dateFormat: DateFormat.d().add_jm(),
        labelRotation: 45,
      ),
      series: <LineSeries<_chartData, DateTime>>[
        LineSeries<_chartData, DateTime>(
          dataSource: data, // Use the data passed to the function
          xValueMapper: (_chartData data, _) => data.x,
          yValueMapper: (_chartData data, _) => data.y,
          markerSettings: const MarkerSettings(
            isVisible: true,
          ),
        ),
      ],
    );
  }

  Future<List<_chartData>> getLiveHistory() async {
    if (isRefreshing == true) {
      liveHistory = await _getLiveHistory();
      return liveHistory;
    } else {
      return liveHistory;
    }
  }

  Future<List<_chartData>> _getLiveHistory() async {
    List<_chartData> liveHistory = [];
    await FirebaseFirestore.instance
        .collection("device/$_uid/sensors/overall/live/")
        .orderBy("time", descending: true)
        .limit(10)
        .get()
        .then((snapshot) {
      final docs = snapshot.docs;
      if (docs.isNotEmpty) {
        debugPrint("-------------------");
        for (var doc in docs) {
          final data = doc.data();
          Timestamp timestamp = data['time'];
          DateTime dateTime = timestamp.toDate();
          // debugPrint(data.toString());
          // debugPrint(dateTime.toString());
          liveHistory.add(_chartData(dateTime, data['power']));
        }
        debugPrint("-------------------");
      }
    });
    return liveHistory;
  }

  Future<List<_chartData>> getDailyHistory() async {
    if (isRefreshing == true) {
      dailyHistory = await _getDailyHistory();
      return dailyHistory;
    } else {
      return dailyHistory;
    }
  }

  Future<List<_chartData>> _getDailyHistory() async {
    List<_chartData> tempDailyHistory = [];
    await FirebaseFirestore.instance
        .collection("device/$_uid/sensors/overall/history/")
        .where("time",
            isGreaterThanOrEqualTo:
                DateTime.now().subtract(const Duration(days: 1)))
        .orderBy("time", descending: true)
        .limit(10)
        .get()
        .then((snapshot) {
      final docs = snapshot.docs;
      if (docs.isNotEmpty) {
        debugPrint("---------Daily--------");
        for (var doc in docs) {
          final data = doc.data();
          Timestamp timestamp = data['time'];
          DateTime dateTime = timestamp.toDate();
          // debugPrint(data.toString());
          // debugPrint(dateTime.toString());
          tempDailyHistory.add(_chartData(dateTime, data['today']));
        }
        debugPrint("-------------------");
      }
    });
    return tempDailyHistory;
  }

  double mapValue(double value, double fromLow, double fromHigh, double toLow,
      double toHigh) {
    return toLow +
        ((value - fromLow) / (fromHigh - fromLow) * (toHigh - toLow));
  }

  @override
  void initState() {
    super.initState();

    _uid = auth.currentUser!.uid;
    // _uid = "b0boEGdZJMYNdxMqi9tQ0UdEXMC3";
    // auth.currentUser!.updateDisplayName("Guest");

    maxLiveUse = getMaxDaily();

    _controller = AnimationController(
      vsync: this, // the SingleTickerProviderStateMixin
      duration: const Duration(seconds: 5),
    )..repeat();

    _docStream = FirebaseFirestore.instance
        .collection('device/$_uid/sensors/overall/live/')
        .orderBy("time", descending: true)
        .snapshots();

    // Listen to changes
    stream = _docStream.listen(
      (snapshot) {
        final docs = snapshot.docs;
        if (docs.isNotEmpty) {
          print("Data updated!");
          print("${docs.first.data()}");
          // Assuming you want to listen to the first document changes, you can implement different logic if needed
          final data = docs.first.data() as Map<String, dynamic>;
          final power = data['power'] as int;
          final timestamp = data['time'] as Timestamp;
          final dateTime = timestamp.toDate();
          final currentTime = DateTime.now();
          if (power > maxLiveUse) {
            maxLiveUse = power;
          }
          final newSpeed =
              mapValue(power.toDouble(), 0, maxLiveUse.toDouble(), 10, 1);
          setState(() {
            liveHistory.insert(0, _chartData(dateTime, newSpeed.toDouble()));

            // check currentTime - dateTime > 1 minute
            if (currentTime.difference(dateTime).inMinutes > 1) {
              setStatus(ReadingStatus.off);
              liveUse = 0;
            } else {
              setStatus(ReadingStatus.on);
              liveUse = power.toDouble();
              _controller.duration = Duration(seconds: newSpeed.toInt());
            }
          });
          debugPrint("maxLiveUse is $maxLiveUse");
          debugPrint("speed is $newSpeed");

          // And finally, restart the animation
          _controller.repeat();
        } else {
          debugPrint("No data");
        }
      },
    )
      ..onDone(() {
        debugPrint("Task Done");
        setStatus(ReadingStatus.off);
      })
      ..onError((error) {
        debugPrint("Error:");
        debugPrint(error.toString());
        setStatus(ReadingStatus.error);
      });
  }

  @override
  Widget build(BuildContext context) {
    // screen width
    final double width = MediaQuery.of(context).size.width;
    DateTime now = DateTime.now();
    DateFormat formatter = DateFormat('dd MMM');
    String formattedDate = formatter.format(now);
    // DatabaseController db = DatabaseController();
    // db.getStructure();
    // auth.signOut();
    return Scaffold(
      appBar: AppBar(
        title: SizedBox(
          width: double.infinity,
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const SizedBox(
                height: 20,
              ),
              Row(
                children: [
                  const Icon(
                    Icons.account_circle,
                    size: 30,
                  ),
                  Text(
                    "  ${widget.credential.user!.displayName}",
                    textAlign: TextAlign.left,
                    style: const TextStyle(
                      fontSize: 20,
                      fontWeight: FontWeight.w400,
                    ),
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
      endDrawer: Drawer(
        child: ListView(
          padding: EdgeInsets.zero,
          children: const [
            ListTile(
              leading: Icon(Icons.settings),
              title: Text('Settings'),
            ),
          ],
        ),
      ),
      body: SingleChildScrollView(
        child: Center(
          child: Column(
            children: [
              const SizedBox(
                height: 20,
              ),
              SizedBox(
                width: width,
                child: Row(
                  children: [
                    const SizedBox(
                      width: 20,
                    ),
                    Text(
                      status,
                      style: const TextStyle(
                        fontSize: 40,
                        fontWeight: FontWeight.w400,
                      ),
                    ),
                    const SizedBox(
                      width: 20,
                    ),
                    Text(
                      formattedDate,
                      style: const TextStyle(
                        fontSize: 30,
                        fontWeight: FontWeight.w100,
                        color: Colors.grey,
                      ),
                    ),
                  ],
                ),
              ),
              status == "Live"
                  ? AnimatedBuilder(
                      animation: _controller,
                      builder: (context, child) {
                        return Transform.rotate(
                          angle: _controller.value * 2.0 * 3.14159,
                          child: child,
                        );
                      },
                      child: Image(
                        width: width / 3,
                        image: const AssetImage("asset/images/cog.png"),
                      ),
                    )
                  : Image(
                      width: width / 3,
                      image: const AssetImage("asset/images/cog.png"),
                    ),
              const SizedBox(
                height: 20,
              ),
              SizedBox(
                width: width / 3,
                child: SfLinearGauge(
                  minimum: 0,
                  maximum: (((maxLiveUse + 99) / 100) * 100) + 1,
                  barPointers: [
                    LinearBarPointer(
                      value: liveUse.toDouble(),
                    ),
                  ],
                  axisTrackStyle: const LinearAxisTrackStyle(
                    gradient: LinearGradient(
                      colors: [
                        Colors.green,
                        Colors.yellow,
                        Colors.red,
                      ],
                    ),
                  ),
                  markerPointers: [
                    LinearWidgetPointer(
                      value: liveUse.toDouble(),
                      position: LinearElementPosition.outside,
                      child: Text("${liveUse.toDouble()}"),
                    )
                  ],
                ),
              ),
              const SizedBox(
                height: 20,
              ),
              SizedBox(
                width: width,
                height: 60,
                child: Center(
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      // DropdownButton(items: const [], onChanged: ),
                      ToggleButtons(
                        direction: Axis.horizontal,
                        onPressed: (int index) {
                          setState(
                            () {
                              // The button that is tapped is set to true, and the others to false.
                              for (int i = 0; i < _selectedMenu.length; i++) {
                                _selectedMenu[i] = i == index;
                              }
                              _selectedMenuIndex = _selectedMenu
                                  .indexWhere((element) => element == true);
                            },
                          );
                        },
                        borderRadius:
                            const BorderRadius.all(Radius.circular(8)),
                        constraints: const BoxConstraints(
                          minHeight: 40.0,
                          minWidth: 80.0,
                        ),
                        isSelected: _selectedMenu,
                        children: fruits,
                      ),
                    ],
                  ),
                ),
              ),
              FutureBuilder(
                future: _selectedMenuIndex == 0
                    ? getLiveHistory()
                    : getDailyHistory(),
                builder: (context, snapshot) {
                  // debugPrint("Snapshot is $snapshot");
                  if (snapshot.connectionState == ConnectionState.done) {
                    if (snapshot.hasData) {
                      List<_chartData> data = snapshot.data as List<_chartData>;
                      // isRefreshing = false;
                      return graphTest(
                          data); // Pass the fetched data to graphTest()
                    } else {
                      // Handle the case when there is no data
                      return const Text('No data available');
                    }
                  } else {
                    return const CircularProgressIndicator();
                  }
                },
              ),
            ],
          ),
        ),
      ),
    );
  }
}
