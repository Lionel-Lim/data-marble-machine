import 'package:elow/service/database.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:intl/intl.dart';

class Dashboard extends StatefulWidget {
  final UserCredential credential;
  const Dashboard({super.key, required this.credential});

  @override
  State<Dashboard> createState() => _DashboardState();
}

class _DashboardState extends State<Dashboard>
    with SingleTickerProviderStateMixin {
  late final AnimationController _controller;
  @override
  void initState() {
    super.initState();
    _controller = AnimationController(
      vsync: this, // the SingleTickerProviderStateMixin
      duration: const Duration(seconds: 5),
    )..repeat();
  }

  @override
  Widget build(BuildContext context) {
    // screen width
    final double width = MediaQuery.of(context).size.width;
    DateTime now = DateTime.now();
    DateFormat formatter = DateFormat('dd MMM');
    String formattedDate = formatter.format(now);
    DatabaseController db = DatabaseController();
    db.getData();

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
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
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
                  IconButton(
                    onPressed: () {},
                    icon: const Icon(Icons.menu_sharp),
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
      body: Center(
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
                  const Text(
                    "Live",
                    style: TextStyle(
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
            AnimatedBuilder(
              animation: _controller,
              builder: (context, child) {
                return Transform.rotate(
                  angle: _controller.value * 2.0 * 3.14159,
                  child: child,
                );
              },
              child: const Image(
                width: 150,
                image: AssetImage("asset/images/cog.png"),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
