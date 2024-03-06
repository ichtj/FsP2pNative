[![](https://jitpack.io/v/ichtj/FsP2pNative.svg)](https://jitpack.io/#ichtj/FsP2pNative)
Add it in your root build.gradle at the end of repositories:

	dependencyResolutionManagement {
		repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
		repositories {
			mavenCentral()
			maven { url 'https://jitpack.io' }
		}
	}
Step 2. Add the dependency

	dependencies {
	        implementation 'com.github.ichtj:FsP2pNative:1.0.4'
	}