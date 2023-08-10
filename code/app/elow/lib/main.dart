import 'package:elow/screen/dashboard.dart';
import 'package:elow/screen/login.dart';
import 'package:flutter/material.dart';
import 'package:firebase_core/firebase_core.dart';
import 'firebase_options.dart';
import 'package:firebase_auth/firebase_auth.dart';

late final FirebaseApp app;
late final FirebaseAuth auth;
//
//Remove the line below when you are ready to test with a real user.
//
late final UserCredential tempCredential;

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();

  // We store the app and auth to make testing with a named instance easier.
  app = await Firebase.initializeApp(
    options: DefaultFirebaseOptions.currentPlatform,
  );
  auth = FirebaseAuth.instanceFor(app: app);
  //
  //Remove the line below when you are ready to test with a real user.
  //
  // tempCredential = await auth.signInWithEmailAndPassword(
  //     email: "test@dylim.dev", password: "123456");

  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  // This widget is the root of your application.
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'ELOW',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.grey),
        useMaterial3: true,
      ),
      // home: Login(auth: auth),
      //
      //Remove the line below when you are ready to test with a real user.
      //
      // home: Dashboard(credential: tempCredential),
      initialRoute: '/login',
      onGenerateRoute: (RouteSettings settings) {
        switch (settings.name) {
          case '/login':
            FirebaseAuth auth = FirebaseAuth.instance;
            return MaterialPageRoute(builder: (context) => Login(auth: auth));
          case '/main':
            final UserCredential credential =
                settings.arguments as UserCredential;
            return MaterialPageRoute(
                builder: (context) => Dashboard(credential: credential));
          default:
            return null; // Throw an error page or handle unknown routes
        }
      },
    );
  }
}
