package com.library.natives;

import androidx.annotation.IntDef;


import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

@IntDef({PutType.EVENT,PutType.METHOD,PutType.UPGRADE,PutType.SETPERTIES,PutType.GETPERTIES,PutType.UPLOAD,PutType.BROADCAST/*,PutType.NOTIFY*/})
@Retention(RetentionPolicy.SOURCE)
public @interface IPutType {
}
