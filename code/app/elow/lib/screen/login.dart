import 'package:cloud_firestore/cloud_firestore.dart';
import 'package:elow/screen/dashboard.dart';
import 'package:flutter/material.dart';
import 'package:firebase_auth/firebase_auth.dart';

class Login extends StatefulWidget {
  final FirebaseAuth auth;

  const Login({super.key, required this.auth});

  @override
  State<Login> createState() => _LoginState();
}

class _LoginState extends State<Login> {
  String parseFirebaseAuthExceptionMessage(
      {String plugin = "auth", required String? input}) {
    if (input == null) {
      return "unknown";
    }
    // https://regexr.com/7en3h
    String regexPattern = r'(?<=\(' + plugin + r'/)(.*?)(?=\)\.)';
    RegExp regExp = RegExp(regexPattern);
    Match? match = regExp.firstMatch(input);
    if (match != null) {
      return match.group(0)!;
    }
    return "unknown";
  }

  Future<UserCredential> login(
      String email, String password, BuildContext context) async {
    debugPrint("login called");
    try {
      final credential = await widget.auth
          .signInWithEmailAndPassword(email: email, password: password);
      return credential;
    } on FirebaseAuthException catch (e) {
      final code = parseFirebaseAuthExceptionMessage(input: e.message);
      if (code == 'user-not-found') {
        debugPrint('No user found for that email.');
      } else if (code == 'wrong-password') {
        debugPrint('Wrong password provided for that user.');
      } else {
        debugPrint(code);
      }
      ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text(code)));
    }
    throw Exception("login failed");
  }

  Future<UserCredential> signup(
      String name, String email, String password, BuildContext context) async {
    debugPrint("signup called");
    try {
      final credential = await widget.auth
          .createUserWithEmailAndPassword(email: email, password: password);
      await credential.user?.updateDisplayName(name);
      return credential;
    } on FirebaseAuthException catch (e) {
      final code = parseFirebaseAuthExceptionMessage(input: e.message);
      if (code == 'email-already-in-use') {
        debugPrint('Email already in use.');
      } else if (code == 'weak-password') {
        debugPrint('Password is too weak.');
      } else {
        debugPrint(code);
      }
      ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text(code)));
    }
    throw Exception("signup failed");
  }

  Future<bool> checkUserDevice(String uid) async {
    bool isDeviceExist = false;
    // get device/$uid document
    await FirebaseFirestore.instance
        .collection("device")
        .doc(uid)
        .get()
        .then((snapshot) {
      print(snapshot.data());
      if (snapshot.exists) {
        isDeviceExist = true;
      } else {
        isDeviceExist = false;
      }
    });
    return isDeviceExist;
  }

  Widget inputs(bool isCreateAccount) {
    return Column(
      children: [
        isCreateAccount
            ? SizedBox(
                width: 250,
                height: 50,
                child: TextField(
                  controller: nameInputController,
                  obscureText: false,
                  decoration: const InputDecoration(
                    border: OutlineInputBorder(),
                    labelText: 'Name',
                  ),
                ),
              )
            : const SizedBox.shrink(),
        const SizedBox(
          height: 20,
        ),
        SizedBox(
          width: 250,
          height: 50,
          child: TextField(
            controller: emailInputController,
            obscureText: false,
            autofillHints: const [AutofillHints.email],
            keyboardType: TextInputType.emailAddress,
            decoration: const InputDecoration(
              border: OutlineInputBorder(),
              labelText: 'Email',
            ),
          ),
        ),
        const SizedBox(
          height: 20,
        ),
        SizedBox(
          width: 250,
          height: 50,
          child: TextField(
            textInputAction: TextInputAction.next,
            controller: passwordInputController,
            obscureText: true,
            autofillHints: const [AutofillHints.password],
            keyboardType: TextInputType.visiblePassword,
            decoration: const InputDecoration(
              border: OutlineInputBorder(),
              labelText: 'Password',
            ),
          ),
        ),
        isCreateAccount
            ? const SizedBox(
                width: 250,
                child: Text(
                  "Min 6 characters",
                  textAlign: TextAlign.end,
                  style: TextStyle(
                    color: Colors.grey,
                    fontSize: 12,
                  ),
                ),
              )
            : const SizedBox.shrink(),
        const SizedBox(
          height: 20,
        ),
        isCreateAccount // Button either sign up or login
            ? SizedBox(
                // Sign up button
                width: 250,
                height: 50,
                child: ElevatedButton(
                  onPressed: () async {
                    if (nameInputController.text == "" ||
                        emailInputController.text == "" ||
                        passwordInputController.text == "") {
                      ScaffoldMessenger.of(context).showSnackBar(
                        const SnackBar(
                          content: Text("Please fill in all fields"),
                        ),
                      );
                    } else {
                      final UserCredential credential = await signup(
                          nameInputController.text,
                          emailInputController.text,
                          passwordInputController.text,
                          context);
                      if (context.mounted) {
                        Navigator.pushAndRemoveUntil(
                          context,
                          MaterialPageRoute(
                            builder: (context) =>
                                Dashboard(credential: credential),
                          ),
                          (route) => false,
                        );
                      }
                    }
                    debugPrint("Sign up button pressed");
                  },
                  child: const Text('Sign Up'),
                ),
              )
            : SizedBox(
                // Login button
                width: 250,
                height: 50,
                child: ElevatedButton(
                    onPressed: () async {
                      if (emailInputController.text == "" ||
                          passwordInputController.text == "") {
                        ScaffoldMessenger.of(context).showSnackBar(
                          const SnackBar(
                            content: Text("Please fill in all fields"),
                          ),
                        );
                      } else {
                        final UserCredential credential = await login(
                            emailInputController.text,
                            passwordInputController.text,
                            context);
                        bool isDeviceExist =
                            await checkUserDevice(credential.user!.uid);
                        if (context.mounted) {
                          if (isDeviceExist) {
                            Navigator.pushNamedAndRemoveUntil(
                              context,
                              '/main',
                              (route) => false,
                              arguments: credential,
                            );
                          } else {
                            Navigator.pushNamed(
                              context,
                              '/devicesetup',
                            );
                          }
                        }
                      }
                    },
                    child: const Text('Login')),
              ),
      ],
    );
  }

  final nameInputController = TextEditingController();
  final emailInputController = TextEditingController();
  final passwordInputController = TextEditingController();
  bool isCreateAccount = false;

  @override
  void dispose() {
    nameInputController.dispose();
    emailInputController.dispose();
    passwordInputController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      extendBodyBehindAppBar: true,
      appBar: AppBar(
        elevation: 0,
        backgroundColor: Colors.transparent,
        toolbarHeight: 150,
        title: const Image(
          width: 100,
          isAntiAlias: true,
          image: AssetImage('asset/images/logo.png'),
        ),
      ),
      body: Container(
        decoration: const BoxDecoration(
          image: DecorationImage(
            image: AssetImage('asset/images/background.png'),
            fit: BoxFit.cover,
            opacity: 1,
          ),
        ),
        child: SafeArea(
          child: Center(
            child: Container(
              decoration: BoxDecoration(
                color: Colors.white.withOpacity(0.9),
                borderRadius: BorderRadius.circular(20),
              ),
              width: 300,
              child: SingleChildScrollView(
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    inputs(isCreateAccount),
                    Row(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        isCreateAccount
                            ? const Text("Do you have an account?")
                            : const Text("Don't have an account?"),
                        TextButton(
                          onPressed: () {
                            setState(() {
                              isCreateAccount = !isCreateAccount;
                            });
                          },
                          child: isCreateAccount
                              ? const Text("Sign in")
                              : const Text("Sign up"),
                        ),
                      ],
                    ),
                    Row(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        const Text("Exhibition guest?"),
                        TextButton(
                          child: const Text("Login with test account"),
                          onPressed: () async {
                            final UserCredential credential = await login(
                                "temp@dylim.dev", "temp!temp!", context);
                            if (context.mounted) {
                              Navigator.pushNamedAndRemoveUntil(
                                context,
                                '/main',
                                (route) => false,
                                arguments: credential,
                              );
                            }
                          },
                        ),
                      ],
                    )
                  ],
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }
}
